# FlatSat Satellite Learning Kit: Overview

Welcome to the **FlatSat Learning Kit** documentation. This kit is a comprehensive educational platform designed to provide a hands-on understanding of satellite engineering. By laying out the core components of a CubeSat on a flat, accessible plane, students and engineers can easily study, program, and test the fundamental systems that make up a modern small satellite.

The kit is divided into two primary segments: the **Space Segment** (the FlatSat itself) and the **Ground Segment** (the station used to communicate with the satellite).

---

## System Architecture

The FlatSat architecture centers around a main circuit board where various subsystems are integrated through a standardized **PC104 Bus**. A key feature of this design is its **centralized control logic**: while most subsystems have their own dedicated processors, the **Electrical Power System (EPS)** is controlled directly by the **On-Board Computer (OBC)** via a shared I2C bus.

### Key Subsystems

#### 1. [On-Board Computer (OBC)](subsystems/obc.md)
The **OBC** serves as the primary "brain" for the entire satellite. In addition to managing mission logic and data handling, it acts as the controller for the EPS hardware.

*   **Core MCU:** STM32F429ZI.
*   **Key Functions:** High-level processing, GPS data acquisition, and executing control logic for the EPS sensors and switches.
*   **Storage:** Supports external SD Card and 2MB of internal Flash memory.
*   **[Technical Details & Example Code](./subsystems/obc.md)**

#### 2. [Electrical Power System (EPS)](subsystems/eps.md)
The **EPS** is the hardware layer responsible for harvesting, storing, and distributing power. **Unlike other modules, the EPS does not have its own microcontroller;** instead, all of its monitoring sensors and power switches are connected directly to the OBC's I2C bus.

*   **Components:** Li-ion 18650 battery (17Wh+), solar charging simulation, and a network of I2C-controlled sensors.
*   **Control Mechanism:** The OBC MCU directly toggles the power switches (ADM1177) and reads telemetry from current, voltage, and temperature sensors (INA226, TMP102) over I2C.
*   **Key Functions:** Battery management, power rail switching, and real-time power telemetry.
*   **[Technical Details & Example Code](./subsystems/eps.md)**

#### 3. [Communication Subsystem (COM)](subsystems/communication.md)
The **COM** module handles the wireless link between the FlatSat and the Ground Station, allowing for telecommands to be received and telemetry to be sent back to Earth.
*   **Core MCU:** STM32F411RE.
*   **Radio Module:** RFM98PW (433 MHz).
*   **Key Functions:** Radio frequency modulation, KISS protocol implementation, and data packet handling.
*   **[Technical Details & Example Code](./subsystems/communication.md)**

#### 4. [Camera Payload](subsystems/payload.md)
The **Payload** represents the "mission" of the satellite, consisting of an imaging system used to capture and process visual data.

*   **Specifications:** 5 Megapixel resolution.
*   **Key Functions:** Image capture and data transfer to the OBC via the SPI protocol.
*   **[Technical Details & Example Code](./subsystems/payload.md)**

#### 5. [Ground Segment (GS)](subsystems/ground-segment.md)
The **Ground Segment** is the terrestrial hardware that allows users to interact with the FlatSat, mimicking a real-world satellite ground station.

*   **Core MCU:** STM32F411RE.
*   **Radio Module:** RFM98PW (433 MHz).
*   **Key Functions:** Receiving satellite telemetry and transmitting control commands via a PC interface (USB-C).
*   **[Technical Details & Example Code](./subsystems/ground-segment.md)**

### [The PC104 Expansion Bus](subsystems/payload.md#pc104-payload-expansion)

The FlatSat Learning Kit is designed for modularity and extensibility, featuring **two dedicated PC104 bus slots**,. These slots provide a standardized interface that allows users to integrate their own custom subsystems or additional mission payloads into the satellite's architecture.

Each PC104 interface provides a comprehensive suite of power and data connections to ensure seamless integration with the existing satellite segments:

*   **Power Distribution:** Includes **two controllable power channels**, allowing users to manage the energy consumption of their custom hardware effectively,.
*   **High-Speed Data Links:** Provides direct **UART** and **CAN Bus** connections to the On-Board Computer (OBC) for robust command and data handling,.

---

## Learning Objectives

Through this documentation and the associated hardware, you will learn:

*   **Unified System Control:** Managing multiple hardware subsystems (OBC and EPS) from a single powerful microcontroller.
*   **Embedded Programming:** Developing for STM32 microcontrollers in a space-system context.
*   **Communication Protocols:** Implementing UART, I2C, SPI, and CAN bus for inter-subsystem and sensor communication.
*   **Power Management:** Directly monitoring energy consumption and managing battery cycles through I2C sensor integration.
*   **RF Communications:** Understanding 433MHz radio links and packet protocols like KISS.