Nota: per la compilazione degli esempi 03-* sulle macchine del laboratorio virtuale è necessario utilizzare il seguente comando:

gcc -o <nomeeseguibile> <nomesorgente> -lrt

Quello che cambia rispetto ai comandi che abbiamo sempre utilizzato per compilare tutti gli esempi precedenti è l'aggiunta dell'opzione "-lrt". Questa opzione forza gcc a effettuare il linking con la libreria runtime del C. A seconda delle versioni di gcc questa libreria può essere utilizzata per default, senza esplicitare l'opzione "-lrt".

Sulle macchine del laboratorio virtuale l'opzione "-lrt" deve essere utilizzata. In caso contrario il linker non saprà dove trovare le funzioni shm_open() e shm_unlink(), producendo l'errore:
/usr/bin/ld: /tmp/ccGOibBB.o: in function `main':
03-shm-consumer.c:(.text+0x30): undefined reference to `shm_open'
/usr/bin/ld: 03-shm-consumer.c:(.text+0x9f): undefined reference to `shm_unlink'
collect2: error: ld returned 1 exit status
