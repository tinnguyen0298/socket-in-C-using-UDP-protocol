# socket-in-C-using-UDP-protocol
1. Compile the code
   Server Side: **gcc -o udpserver udpserver.c**
   Client Side: **gcc -o udpclient udpclient.c**
2. Run the server:
   **./udpserver <loss_probability> <protocol_type>**
   Example: To send a file named test.pdf using Stop-and-Wait protocol with a 10% packet loss:
   ./udpserver 0.1 1
3. Run the client
   **./udpclient <server_hostname> <file_name> <protocol_type>**
   Example: If the server is running on the local machine (127.0.0.1) and you want to use the Stop-and-Wait protocol
   ./udpclient 127.0.0.1 test_file.pdf 1
4. Verify transfer
   On the client side, after the file transfer is complete, you should see a file named **received_file**. This file should be a binary copy of the original PDF file sent from the server.

Open the **received_file** on the client machine to verify that the PDF was transferred correctly.
