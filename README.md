# Backup manager

## Introduction
This project provides a Client-Server application that performs an incremental backup of a given folder, and lets you restore the version of a given date.  
For each day, the last version of each file is permanently stored to allow the restore operation.

### Security warning

This project's objective is to test our skills with a simple Client-Server interaction in a multithreaded environment, so security issues have not been taken in consideration: packets are not encrypted and passwords are stored in plain text.

## Database
The database, based on sqlite3, is composed of a users table and one table for each user.

### Users
| username | password |
| -------- | -------- |
| TEXT     | TEXT     |

### Client table (name = "_" + machine-id)
| filename | timestamp	| file | size | action | hash | last_write_time | permissions	|
| -------- | ----------	| ---- | ---- | ------ | ---- | --------------- | ------------- |
| TEXT     | TEXT		| BLOB | INT  | INT    | TEXT | TEXT		 	| TEXT			|


## Installation
Developed on Linux environment (Ubuntu 20.04).  
Clion required.  

Install the following libraries:
- Boost		-> sudo apt-get install libboost-all-dev
- libSSL	-> sudo apt-get install libssl-dev
- SQLite3	-> sudo apt-get install libsqlite3-dev

Launch Clion and import Client and Server Folders.

## How to use?
Launch Server and Client independently through the Clion interface.  
If it's your first time launching the Client, it will ask you for the folder
to monitor and a password, the username will be automaticaly created through the
linux machine-id.  
Your folder will be constantly monitored and the data will be automatically sent 
to the server. 

### Possible Client commands:
- "q" close the program
- "r" restore a previous version of the monitored folder,
	ask for a specific date to restore ("YYYY-MM-DD" format) and the folder
	in which to store the backup.  

### Debug mode
By changing the *_DEBUG* value in Debug.h (present both in Client and Server folder) from 0 to 1, it's possible to show additional messages that describe the exchange of data between Client and Server, as well as other useful information.

## Authors
Bruno Andrea  
Chetta Cosimo   
Giorgino Luca   