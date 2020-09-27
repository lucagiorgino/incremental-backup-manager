# Backup manager

## Introduction



<!--
Descrizione del progetto
-->

### Security warning

This project objective is to test our skill with a simple Client-Server interaction
in a multithreaded environment. Security issue have not been taken in consideration

## Database
The database, based on sqlite3, is composed of a users table and one table for each client.

### Users
| username | password |
| TEXT     | TEXT     |

### Client table (name = _machine-id)
| filename | timestamp | file | size | action | hash | last_write_time | permissions |
| TEXT     | TEXT	| BLOB | INT  | INT    | TEXT | TEXT		  | TEXT	|


## Installation
Tested only on Linux environment (Ubuntu 20.1).
Clion required.

Install the following libraries:
- Boost	-> sudo apt-get install libboost-all-dev
- libssl 	-> sudo apt-get install libssl-dev
- sqlite3	-> sudo apt-get install libsqlite3-dev

Launch Clion and import Client and Server Folders.

## How to use?
Launch Server and Client independently through the Clion interface.

If it's your first time launching the Client, it will ask you for the folder
to monitor and a password, the username will be automaticaly created through the
linux machine-id.

Your folder will be constantly monitored and the data will be automatically sent 
to the server. 

Possible Client commands:
- "q" close the program
- "r" restore a previous version of the monitored folder,
	ask for a specific date to restore ("YYYY-MM-DD") and the folder
	in which to store the backup.

## Authors
Bruno Andrea
Chetta Cosimo 
Giorgino Luca 
