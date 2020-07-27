Da rivedere

- Authentication
	- Request: /login
	- Response: ok/denied/create new ???
- Select folder for backup (se vi è un backup in corso si attende la fine e si cambia cartella)
	- Request: /folder/backup/:folderName
	- Response: ok/failure
- Add folder to backup (crea una nuova cartella collegata all'account)
	- Request: /folder/new/:folderName
	- Response: ok/failure
- Check if file is in backup
	- Request: /file/:hashedFile
	- Response: ok/no/failure
- Delete file/folder
	- Request/file/delete/:hashedContent
	


