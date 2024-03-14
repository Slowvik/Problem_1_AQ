Stepwise compile instructions on Windows:
1. Compile the server files (in folder Server N) with "g++ server/N.cpp -lws2_32 -o server/N" where /N needs to be replaced with the number (1, 2 or 3).
2. Generating new text files for the servers to read (if required): Navigate to the root (/src/Problem 1) and compile server_file_generator.cpp as follows: "g++ server_file_generator.cpp  -o server_file_generator". The executable takes one parameter from the command line (server ID, which in this case is 1, 2 or 3). Run it so: "./server_file_generator.exe N" where N is either 1, 2 or 3. The text file then needs to be copied over into the correct server folder manually. Text files with 100 integers each are already present.
3. Start up the servers with 2 command line parameters: filename for reading integers and port number as follows: "./server1.exe server1.txt 8000"
4. Compile the client by navigating to the client folder and using "g++ --std=c++20 client_starter.cpp -lws2_32 -o client"
5. Run the client with 1+N command line parameters. The first parameter is the number of servers, the next N parameters are the port numbers for all the servers. For example: "./client.exe 3 8000 8080 4000"
6. If the client is run before the servers are up, it will lead to a socket error when it tries to connect to the server.
7. The IP for all servers currently is 127.0.0.1 (localhost on windows)

Notes:
1. Currently in the process of doing away with multiple serverN.h files and generating multiple executables with different names using the same server_generator file and then running them with different parameters for server ID and port.
2. The server code can be improved, expected to be rewritten in a few days.
3. Since we are not using STL data structures, I have used arrays with predefined sizes, given the nature of the data. These sizes are declared using the #define directive at the top of the client.h file. They need to be increased if the test dataset is increased, or we will encounter errors related to bad reading from the memory.

Notes on the algorithm:
1. Currently, connecting to the servers happens sequentially during the initialisation of the client. There is a possibility to make connection asynchronous using one thread per server.
2. Data is read line by line by the servers, converted to bytes, and sent to the client.
3. The client reads data in chunks of 20 bytes (currently set as the sender buffer size, can be changed). Reading data is asynchronous and happens concurrently with 1 thread per connection.
4. To maintain a real-time list of sorted integers, we note that the data received from one server is already sorted (in ascending order). We take advantage of this by checking for a suitable insertion position from the back of the queue of integers. In case there is a duplicate integer, we ignore it (and copy back the whole array that we had copied forward to prepare for insertion). Unless there are a large number of common integers, this is the fastest way to insert a new integer in a sorted list.
5. Clearly, inserting an integer into the merged sorted list is a critical section, hence it is protected by a semaphore (C++20). Each thread tries to acquire the semaphore with try_acquire() and moves on to reading more data if it cannot. This makes sense because a blocking call would create a bottleneck with all threads waiting here and not reading data from their respective servers.
6. To make things faster, each thread maintains a "queue" of integers it receives from the server, until it can acquire the semaphore for the critical section. When it acquires the semaphore, it inserts the "front" element from its "queue" into the sorted list and releases the semaphore.
7. When the thread encounters a "0" in the list of integers, it knows that the data stream has ended.
8. Once all the threads are done, we join them with the main thread and print the sorted list of integers.
