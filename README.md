# Tema3APD-DistributedTextProcessor
[Tema3 Algoritmi Paraleli si Distribuiti (2020-2021, seria CB)] 

Tema presupune implementarea unui procesor de texte distribuit folosind MPI si pthreads. <br>
Enunt: https://curs.upb.ro/pluginfile.php/471635/mod_resource/content/1/Tema%203.pdf

## Compilare si Rulare
```shell
     make & make run
```

## Implementare
Am inceput prin a creea 5 noduri MPI, primul cu rank=0 reprezentand Master-ul
si restul workeri. 

In nodul `Master` se pornesc 4 thread-uri care se ocupa fiecare
cu un tip de paragraf. Fiecare thread citeste din fisierul de intrare si formeaza,
linie cu linie, paragraful care va fi trimis workerului corespunzator. <br> Acesta
trimite dimensiunea si paragraful, tinand minte si numarul acestuia din fisier.
Primeste de la worker paragraful procesat si il adauga intr-un map in care 
thread-ul pastreaza toate paragrafele procesate, avand ca cheie numarul lor 
din fisier. Cand se termina citirea pentru toate thread-urile, se trimite un 
mesaj care contine -1 care anunta workeri ca s-au prelucrat toate paragrafele.

Avand o variabila globala `current`, se parcurge fiecare numar din intervalul
[1, num_paragraphs] si se gaseste map-ul care contine paragraful si acesta 
este scris in fisier. Astfel se asigura scrierea in ordine a textelor.

`Workeri` asteapta paragrafe de prelucrat pana primesc -1. In functie de workerul
in care ne aflam, paragrafele se prelucreaza conform regulilor pentru horror, 
comedy, fantasy si science-fiction, aplicand functiile care modifica siruri in
mod corespunzator. Apoi ce s-a prelucrat se trimite inapoi la Master pt a fi
afisat in fisier.

MENTIUNE: Nu am implementat separarea logicii de citire si procesare in workeri.
