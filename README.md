# 🌐 TFTP - Trivial File Transfer Protocol 

## 🚀 Overview

This project implements a **TFTP (Trivial File Transfer Protocol)** based file transfer system using **UDP sockets in C**.

It consists of a **client and server**, enabling file upload and download operations using TFTP protocol concepts such as RRQ (Read Request), WRQ (Write Request), DATA, ACK, and ERROR packets.

---

## 🎯 Features

### 📡 Server

* ✅ Handles multiple client requests
* ✅ Supports RRQ (file download)
* ✅ Supports WRQ (file upload)
* ✅ Sends DATA packets block-by-block
* ✅ Handles ACK validation and retransmission
* ✅ Sends ERROR packets for invalid requests

### 💻 Client

* ✅ Command-line interface (CLI)
* ✅ Connect to server using IP
* ✅ Upload file (`put`)
* ✅ Download file (`get`)
* ✅ Change transfer mode (`default`, `octet`, `netascii`)
* ✅ Validate IP address and file presence

---

## 🧠 Concepts Used

* UDP Socket Programming
* Client-Server Architecture
* TFTP Protocol (RRQ, WRQ, DATA, ACK, ERROR)
* File Handling (read/write system calls)
* Byte Order Conversion (`hton`, `ntoh`)
* Packet Struct Design
* Retransmission & Reliability Handling

---

## ⚙️ Technologies Used

* **Language:** C
* **Platform:** Linux
* **Protocol:** UDP
* **Compiler:** GCC

---

## 🏗️ Project Structure

```id="tftpstruct"
TFTP/
│── tftp_server.c
│── tftp_client.c
│── tftp.c
│── tftp.h
│── tftp_client.h
```

---

## ▶️ How to Run

### 1. Compile Server

```bash id="tftp1"
gcc tftp_server.c tftp.c -o server
```

### 2. Compile Client

```bash id="tftp2"
gcc tftp_client.c tftp.c -o client
```

---

### 3. Run Server

```bash id="tftp3"
./server
```

---

### 4. Run Client

```bash id="tftp4"
./client
```

---

## 💻 Client Commands

```bash id="tftp5"
connect <server-ip>    # Connect to server
put <filename>         # Upload file to server
get <filename>         # Download file from server
mode <type>            # Change mode (default/octet/netascii)
help                   # Show commands
quit / bye             # Exit
```

---

## 📦 Working Flow

### Upload (WRQ)

1. Client sends WRQ request
2. Server sends ACK(0)
3. Client sends DATA blocks
4. Server sends ACK for each block
5. Transfer ends when last block < block size

### Download (RRQ)

1. Client sends RRQ request
2. Server sends DATA blocks
3. Client sends ACK for each block
4. Transfer completes after final block

---

## ⚠️ Limitations

* ❌ No encryption or security
* ❌ Single-threaded server
* ❌ Limited error recovery (basic retransmission only)

---

## 🔮 Future Enhancements

* 🔹 Add timeout & retry mechanism improvement
* 🔹 Multi-client support using threads
* 🔹 Add logging system
* 🔹 Secure transfer (TLS-based)

---

## 🧩 Challenges Faced

* Handling packet loss and retransmission
* Managing block numbers correctly
* Implementing different transfer modes
* Synchronizing DATA and ACK packets

---

## 📚 Learning Outcomes

* Strong understanding of UDP socket programming
* Hands-on implementation of TFTP protocol
* Experience with reliable data transfer over unreliable protocol
* Improved debugging of network applications

---

## 📌 Author

**Shubham Chaudhari**
