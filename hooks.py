import os
import zipfile
import glob
import logging

log = logging.getLogger("mkdocs.hooks")

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
                with zipfile.ZipFile(file_path, 'r') as zip_ref:
                    zip_ref.extractall(output_dir)
                log.info(f"Successfully extracted {file_name}.")
            except zipfile.BadZipFile:
                log.error(f"Failed to extract {file_name}. It may not be a valid zip format.")
            except Exception as e:
                log.error(f"Error extracting {file_name}: {e}")

