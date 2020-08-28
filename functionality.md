**Server**
- pool di thread
- struttura dati per ogni utente che contiene nome, password, cartella e file
contenente nomefile/hash

**Client**
- FileWatcher e rispettivo thread
- ista di azioni (FileStatus) per ogni file/cartella, le azioni per contenuti diversi vengono
eseguite in parallelo. Le azioni sullo stesso contenuto invece vengono eseguite in
modo sincrono, aspettando la conferma dal server. Le azioni sulle cartelle sono bloccanti




**Procedimento:**
- Client avvia applicazione
- Esegue login tramite username, password
- Il server invia albero con path file/cartelle e hash
- client avvia il file watcher con le informazioni
- file watcher itera a intervalli sull'albero e setta i FileStatus
- per ogni path, il file watcher aggiunge azione alla lista di azioni in base al FileStatus:
	- created: chiama api created con path, contenuto
	- modified: chiama api modified con path, contenuto
	- deleted: chiama api deleted con path, contenuto
- periodicamente il client esegue la lista di azioni inviando una richiesta per ogni azione
- per ogni richiesta ricevuta, il server la esegue e crea l'hash se necessario


!!! L'hash deve essere indipendente dai metadati(?)
!!! Se al server arriva prima il file

