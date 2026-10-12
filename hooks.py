import os
import zipfile
import glob
import logging
import re
import fnmatch
import urllib.parse
import subprocess
import xml.etree.ElementTree as ET

log = logging.getLogger("mkdocs.hooks")


def resolve_drawio_page_index(source_path, page_identifier):
    """
    Resolve a page identifier (page number, page name, or diagram id)
    to a 1-based page index required by Draw.io CLI (v27+).
    """
    try:
        idx = int(page_identifier)
        return 1 if idx <= 0 else idx
    except (ValueError, TypeError):
        pass

    if os.path.exists(source_path):
        try:
            tree = ET.parse(source_path)
            diagrams = tree.getroot().findall("diagram")
            for i, diag in enumerate(diagrams, 1):
                if diag.get("name") == page_identifier or diag.get("id") == page_identifier:
                    return i
        except Exception as e:
            log.warning(f"Failed to parse Draw.io XML for {source_path}: {e}")
    return 1


def patch_drawio_exporter():
    """
    Patches mkdocs-drawio-exporter to:
    1. Support Draw.io v27+ 1-based page indexing and resolve page names.
    2. Support both Markdown image syntax ![]() and HTML <img> tags.
    3. Normalize relative paths (e.g. handling ../ vs ../../).
    4. Auto-detect draw.io binary on macOS and Linux.
    """
    try:
        import mkdocs_drawio_exporter.exporter as exporter_mod
        from mkdocs_drawio_exporter.exporter import DrawIoExporter, Source
    except ImportError:
        log.warning("mkdocs_drawio_exporter not installed, skipping patch.")
        return

    orig_export_file = DrawIoExporter.export_file

    def patched_export_file(self, source, page_index, dest, config):
        page_idx = resolve_drawio_page_index(source, page_index)
        cmd = [
            config["drawio_executable"],
            "--export", source,
            "--page-index", str(page_idx),
            "--output", dest,
            "--format", config["format"],
        ]
        cmd += config.get("drawio_args", [])
        try:
            self.log.debug(f"Using Draw.io export command: {' '.join(cmd)}")
            return subprocess.call(cmd)
        except Exception:
            self.log.exception("Draw.io export subprocess failed")
            return 1

    DrawIoExporter.export_file = patched_export_file

    def patched_rewrite_image_embeds(self, page_src_path, is_index, output_content, config):
        content_sources = []

        def resolve_source_info(raw_filename, alt_text):
            try:
                filename, page_id = raw_filename.rsplit("#", 1)
            except ValueError:
                filename = raw_filename
                page_id = alt_text if alt_text else 1

            unescaped = urllib.parse.unquote(filename)
            candidate = os.path.normpath(os.path.join(os.path.dirname(page_src_path), unescaped))

            # Normalize if redundant parent dirs (e.g. ../../ from a 1-level deep dir)
            if (candidate.startswith(".." + os.sep) or candidate == "..") and not os.path.exists(os.path.join(self.docs_dir, candidate)):
                fixed = re.sub(r"^(\.\.[/\\])+", "", candidate)
                if os.path.exists(os.path.join(self.docs_dir, fixed)):
                    candidate = fixed

            abs_src_path = os.path.join(self.docs_dir, candidate)
            page_index = resolve_drawio_page_index(abs_src_path, page_id)

            # Calculate correct img_src for destination HTML
            if self.use_directory_urls:
                if is_index:
                    parts = os.path.dirname(page_src_path).split(os.sep) if os.path.dirname(page_src_path) else []
                    depth = len([p for p in parts if p])
                else:
                    parts = os.path.splitext(page_src_path)[0].split(os.sep)
                    depth = len([p for p in parts if p])
            else:
                parts = os.path.dirname(page_src_path).split(os.sep) if os.path.dirname(page_src_path) else []
                depth = len([p for p in parts if p])

            prefix = "../" * depth
            target_file = f"{candidate}-{page_index}.{config['format']}"
            dest_img_src = prefix + target_file

            return filename, candidate, page_index, dest_img_src

        # 1. Process Markdown image syntax ![...](...)
        def replace_md(match):
            raw_filename = match.group("filename")
            alt_text = match.group("alt")
            fn_without_anchor = raw_filename.rsplit("#", 1)[0]
            if fnmatch.fnmatch(fn_without_anchor, config["sources"]):
                filename, candidate, page_index, dest_img_src = resolve_source_info(raw_filename, alt_text)
                source = Source(filename, page_index)
                source.source_rel = candidate
                content_sources.append(source)
                return config["embed_format"].format(img_alt=alt_text, img_src=dest_img_src, content=None)
            return match.group(0)

        output_content = exporter_mod.IMAGE_RE.sub(replace_md, output_content)

        # 2. Process HTML <img> tags
        HTML_IMG_RE = re.compile(
            r"<img\s+(?P<attrs>[^>]*?src=[\"'](?P<src>[^\"']*?\.drawio(?:#[^\"']*)?)[\"'][^>]*?)>",
            re.IGNORECASE
        )

        def replace_html(match):
            raw_attrs = match.group("attrs")
            raw_src = match.group("src")
            alt_m = re.search(r"alt=[\"']([^\"']*)[\"']", raw_attrs)
            alt_text = alt_m.group(1) if alt_m else ""
            src_without_anchor = raw_src.rsplit("#", 1)[0]

            if fnmatch.fnmatch(src_without_anchor, config["sources"]):
                filename, candidate, page_index, dest_img_src = resolve_source_info(raw_src, alt_text)
                source = Source(filename, page_index)
                source.source_rel = candidate
                content_sources.append(source)

                # Replace src attribute with dest_img_src
                new_attrs = re.sub(
                    r'src=[\"\']' + re.escape(raw_src) + r'[\"\']',
                    f'src="{dest_img_src}"',
                    raw_attrs
                )
                return f"<img {new_attrs}>"
            return match.group(0)

        output_content = HTML_IMG_RE.sub(replace_html, output_content)
        return (output_content, content_sources)

    DrawIoExporter.rewrite_image_embeds = patched_rewrite_image_embeds
    log.info("Patched mkdocs-drawio-exporter for Draw.io compatibility.")


patch_drawio_exporter()


def on_startup(command, dirty, **kwargs):
    """
    This hook runs exactly once when MkDocs starts.
    It will NOT re-run during a hot-reload.
    """
    if command == "serve":
        log.info("Starting up! Running custom epro2 extraction script...")

        # Path to the source_docs directory
        base_dir = os.path.dirname(os.path.abspath(__file__))
        source_docs_dir = os.path.join(base_dir, "source_docs")
        extract_target_dir = os.path.join(source_docs_dir, "extracted")

        if not os.path.exists(source_docs_dir):
            log.warning(f"Directory {source_docs_dir} not found.")
            return

        # Find all .epro2 files
        epro2_files = glob.glob(os.path.join(source_docs_dir, "*.epro2"))

        if not epro2_files:
            log.info("No .epro2 files found to extract.")
            return

        # Create extraction target directory if it doesn't exist
        os.makedirs(extract_target_dir, exist_ok=True)

        for file_path in epro2_files:
            file_name = os.path.basename(file_path)
            folder_name = os.path.splitext(file_name)[0]
            output_dir = os.path.join(extract_target_dir, folder_name)

            # Create a folder for this specific project
            os.makedirs(output_dir, exist_ok=True)

            log.info(f"Extracting {file_name} to {output_dir}...")

            try:
                # Treat .epro2 as a zip file and extract it
                with zipfile.ZipFile(file_path, "r") as zip_ref:
                    zip_ref.extractall(output_dir)
                log.info(f"Successfully extracted {file_name}.")
            except zipfile.BadZipFile:
                log.error(f"Failed to extract {file_name}. It may not be a valid zip format.")
            except Exception as e:
                log.error(f"Error extracting {file_name}: {e}")
