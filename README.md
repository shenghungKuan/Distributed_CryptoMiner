This program is an implementation of a miner program that performs the hash inversion technique used in cryptocurrencies like Bitcoin. The goal of the miner is to find a specific nonce that meets a given difficulty requirement (number of consecutive zeros in the front of a hash). This program is built to act as a client which recieves problems from a server, solves them, and returns the solution to the server. The main function starts by establishing a connection with a server and obtaining a problem (task) from it. The program creates multiple threads to perform the mining process in parallel. Each thread is responsible for generating and testing different nonce values. If a thread finds a solution (a hash that satisfies the difficulty requirement), it signals other threads to stop and records the nonce and hash value. Once all threads have finished their work, the program checks if any thread found a solution. If a solution is found, it prints the thread number, nonce, and hash value. It also sends the solution to the server. All of this happens in a loop that will run until the server stops sending problems. Additionally, there is a heartbeat thread that periodically sends heartbeat messages to the server to keep the connection alive and inform the server that the miner is still active. 


## miner  
usage: ./miner [number of threads(integer)] [difficulty(integer)] "[block data]"  
e.g. ./miner 4 24 "hello world!!!"  
description:    
encode the given block data with a nonce that produce "difficulty" zeros multi-threading with "number of threads" threads.

## miner-client 
usage: ./miner-client [host type] [port number]     
e.g. ./miner-client localhost 3001  
description:  
request a task including "block data" and "difficulty" from the server, find the solution(nonce) using multi-threading, and send it back to the server repeatedly.


