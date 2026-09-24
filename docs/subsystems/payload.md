# Payload

## Camera Payload Subsystem

The **Camera Payload** represents the primary "mission" of the FlatSat satellite. It consists of a high-resolution imaging system designed to capture visual data of the "Earth" (or its surroundings) and transmit that data to the On-Board Computer (OBC) for processing and storage.

### Block Diagram

<figure>
<img alt="Payload" src="../../assets/diagram/Flatsat_Block_Diagram.drawio"/>
<caption>Payload Block Diagram</caption>
</figure>

### Technical Specifications

The payload is built to provide high-quality imagery while maintaining a low power profile suitable for small satellite operations:

*   **Resolution:** 5 Megapixels.
*   **Image Format:** Supports compressed **JPEG** output, which is ideal for reducing the data volume transmitted over satellite radio links.
*   **Interface:** Uses a combination of high-speed **SPI** for transferring large image data and **I2C** for camera configuration and control.
*   **Control Logic:** The payload is directly managed by the **STM32F429ZI** MCU on the OBC.

### Hardware Connectivity

The Camera Payload is connected to the OBC via several dedicated pins to ensure high-speed data handling:

| Interface | OBC Pin | Function |
| :--- | :--- | :--- |
| **SPI CS** | **PE7** | Chip Select for the Camera module. |
| **SPI MOSI** | **PB15** | Master Out Slave In for data transmission. |
| **SPI MISO** | **PB14** | Master In Slave Out for data reception. |
| **SPI SCK** | **PB13** | Serial Clock. |
| **I2C SDA** | **PB9** | Serial Data for camera configuration. |
| **I2C SCL** | **PB8** | Serial Clock for camera configuration. |

*Note: In some software implementations, SPI pins may be dynamically remapped to PB3, PB4, and PB5 to avoid conflicts with other SPI peripherals like the Flash memory.*

### Mission Operations

The typical operational flow for the camera payload involves the following steps:

1.  **Initialization:** The OBC initializes the camera sensor via the SPI/I2C interface.
2.  **Capture:** The OBC sends a "take picture" command.
3.  **Buffering:** Image data is read from the camera's internal memory in small chunks (buffers).
4.  **Storage:** The OBC writes these chunks sequentially to the **Micro SD card** (using the PC9/PC10/PC11/PC12 pins) to create a final `.jpg` image file.

---

### Example Code

The following example code demonstrates how to trigger a picture capture and save the resulting file to the on-board SD card.

| Feature | Description | Code Link |
| :--- | :--- | :--- |
| **Camera to SD** | Capture a 5MP image and save it as a JPEG file on the SD card. | [obc_camera2sd.ino](../codes/obc/obc_camera2sd/obc_camera2sd.ino) |

## PC104 Payload Expansion

The payload could be expanded with PC104 header

<figure>
<!-- Full-Board Highlight -->
<svg viewBox="0 0 6300 5400" width="100%" style="border-radius: 8px; margin-bottom: 1rem;" xmlns="http://www.w3.org/2000/svg">
  <defs>
    <mask id="payload-mask">
      <rect width="6300" height="5400" fill="white" opacity="0.3" />
      <rect x="135" y="2715" width="3604" height="1718" fill="white" rx="50" />
    </mask>
  </defs>
  <image href="../../assets/flatsat_board.jpg" width="6300" height="5400" mask="url(#payload-mask)" />
  <rect x="135" y="2715" width="3604" height="1718" fill="none" stroke="#00e5ff" stroke-width="30" rx="50" />
  <text x="135" y="2600" fill="#00e5ff" font-size="200" font-family="sans-serif" font-weight="bold" style="text-shadow: 2px 2px 10px #000, -2px -2px 10px #000, 0 0 20px #000;">Payload Expansion Location</text>
</svg>

<!-- Cropped Section with Labels -->
<svg viewBox="135 2715 3604 1718" width="100%" style="border-radius: 8px; box-shadow: 0 4px 6px rgba(0,0,0,0.1);" xmlns="http://www.w3.org/2000/svg">
  <image href="../../assets/flatsat_board.jpg" width="6300" height="5400" />
  <!-- Highlights inside the crop -->

  
  <rect x="200" y="2820" width="3500" height="200" fill="none" stroke="#ff007f" stroke-width="15" rx="30" />
  <text x="300" y="3150" fill="#ff007f" font-size="120" font-family="sans-serif" font-weight="bold" style="text-shadow: 2px 2px 10px #000, -2px -2px 10px #000, 0 0 20px #000;">PC104 Expansion Headers</text>
</svg>
<caption>PC104 Payload Expansion</caption>
</figure>
<!-- ## GPS Payload -->
