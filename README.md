# Project 4: Distributed CryptoMiner

See spec here: https://www.cs.usfca.edu/~mmalensek/cs521/assignments/project-4.html  

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


