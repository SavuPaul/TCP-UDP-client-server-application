README TEMA 2 - PROTOCOALE DE COMUNICATIE - APLICATIE CLIENT-SERVER TCP & UDP 

!!! Structura de baza a makefile-ului si a modului in care opereaza atat server-ul cat si clientul (subscriber-ul) 
sunt luate din laboratorul 7 !!!

Implementarea consta intr-un server care ramane deschis pana este oprit fortat cu de la stdin cu comanda "exit" si clienti care
se conecteaza la server pe socket-uri TCP si care pot furniza date catre server sau sa primeasca.

Trimiterea datelor este realizata cu functia send_data care trimite mai intai dimensiunea pachetului, urmata de pachet in sine.
Primirea datelor este realizata cu functia receive_data care primeste mai intai dimensiunea pachetului, urmata de pachet in sine.

IMPLEMENTAREA SERVER-ULUI:
Clientii care se conecteaza la server si topic-urile la care acestia sunt abonati, sunt implementate sub forma unor liste cu santinela.
Am ales sa implementez topic-urile sub forma de lista pentru a se intelege mai usor codul (topic-urile puteau fi implementate si sub 
forma unei matrice de caractere).

Server-ul are la baza 3 socket-uri de baza:
    - cel pentru stdin
    - cel UDP
    - cel TCP
La acesti 3 socketi, se mai adauga cate unul pentru fiecare client care vrea sa se conecteze, comunicarea cu fiecare client
facandu-se pe socket-ul asociat conexiunii individuale cu acel client.

Daca server-ul primeste comanda "exit" de la stdin, atunci se va trimite mesajul "exit" tuturor clientilor pentru a-si incheia activitatea.

Daca server-ul primeste date pe socket-ul UDP, inseamna ca a primit un mesaj pe care il receptioneaza si il transforma intr-o variabila de tip
(struct message_udp *) cu scopul de a salva adresa IP si portul astfel incat afisarea mesajelor din clienti sa fie mai usoara. Acest mesaj UDP
este trimis mai departe tuturor clientilor care sunt abonati la topicul mesajului respectiv.

Daca server-ul primeste date pe socket-ul TCP, inseamna ca un client nou vrea sa se conecteze. Se receptioneaza ID-ul, iar daca acesta nu mai
exista deja, clientul este adaugat in lista de clienti a aplicatiei, iar socket-ul lui este adaugat in poll-ul de socket-uri ai server-ului.
Daca ID-ul exista deja, atunci clientul este respins si se inchide socket-ul asociat lui.

Daca server-ul primeste date de pe socket-urile clientilor, atunci acestea pot fi de tip:
    - exit -> Clientul anunta ca se inchide, iar server-ul trebuie sa printeze mesajul aferent, sa stearga clientul din lista de clienti a
              aplicatiei, sa inchida socket-ul pentru conexiune si sa elimine socket-ul din poll-ul server-ului.
    - subscribe -> Clientul da subscribe la un topic, iar acesta trebuie adaugat in lista de topic-uri ale clientului.
    - unsubscribe -> Clientul se dezaboneaza de la un topic, acesta fiind sters din lista de topic-uri ale clientului.


IMPLEMENTAREA SUBSCRIBER-ULUI:
Clientul are la baza 2 socket-uri:
    - cel TCP, prin care comunica cu server-ul
    - cel pentru stdin

Daca clientul primeste date de la stdin, comenzile pot fi:
    - exit -> clientul transmite mesaj catre server pentru a se deconecta si inchide socket-ul. Serverul printeaza faptul ca subscriber-ul 
              s-a deconectat.
    - subscribe -> clientul transmite topicul la care doreste sa se aboneze si afiseaza un mesaj de confirmare ("Subscribed to topic <TOPIC>").
    - unsubscribe -> clientul transmite topicul de la care vrea sa se dezaboneze si afiseaza un mesaj de confirmare ("Unsubscribed from topic
                     <TOPIC>").

Daca clientul primeste date de pe socket, adica de la server, acestea pot fi:
    - exit -> atunci cand server-ul este oprit cu comanda "exit" de la stdin, clientul trebuie sa isi incheie activitatea
    - mesaj udp -> aceste mesaje sunt de mai multe tipuri, output-ul fiind construit in functie de tipul mesajului, conform cerintei. Pentru 
                   afisarea adreselor sursa si a porturilor, sunt folosite cele doua campuri din structura mesajului udp.