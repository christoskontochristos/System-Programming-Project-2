Χρήστος Κοντοχρήστος 1115202000090


Έχω βάλει το backlog value να είναι 128.

Σχολιασμός Κώδικα:

Όταν πρόκειται για το poll command τότε ο server διατρέχει τον κοινό buffer και στέλνει στον jobCommander ένα array που περιέχει strings τύπου <jobID, job>. Ο jobCommander στη συνέχεια διαβάζει αυτόν τον array και τυπώνει τα στοιχεία του. Αν ο buffer είναι άδειος τότε ο jobCommander δεν εκτυπώνει τίποτα.

Όταν πρόκειται για issueJob command, ο server στέλνει πίσω στον jobCommander έναν αριθμό πρίν στείλει οποιοδήποτε μήνυμα για να μπορέσει ο jobCommander να διαχειριστεί τι μήνυμα πρέπει να διαβάσει από το socket που έχει στείλει ο server. Αυτός ο αριθμός αντιπροσωπεύει την κατάσταση του job. Δηλαδή αν ο αριθμός που στείλει ο server είναι το 1, τότε ο jobCommander που έχει στείλει ένα issueJob αίτημα ξέρει οτι 
το job που έστειλε στον server να εκτελεστεί διακόπηκε από ένα stop job command και επομένως ξέρει οτι η δουλεία που έστειλε δεν θα εκτελεστεί και με αυτόν τον τρόπο δεν περιμένει να διαβάσει το output της δουλειάς και απλά τερματίζει. Αν ο server στέιλει στον jobCommander τον αριθμό 2 τότε ο jobCommander αναγνωρίζει οτι η δουλειά που έστειλε στον server εκτελέστηκε και έτσι στη συνέχεια διαβάζει 
το output της δουλειάς. Τέλος αν ο server στείλει στον jobCommander τον αριθμό 3 τότε ο jobCommander αναγνωρίζει οτι η δουλειά που έστειλε στον server για να εκτελεστεί με ένα issueJob αίτημα, δεν θα εκτελεστεί επειδή ο server τερμάτισε πρίν η δουλεία προλάβει να εκτελεστεί (δηλαδή ο server τερμάτισε ένω η δουλειά ήταν ακόμα στον κοινό buffer), επομένως ξέρει οτι δεν πρέπει να διαβάσει το job output αλλά πρέπει να διαβάσει το "SERVER TERMINATED BEFORE EXECUTION" μήνυμα που έχει στείλει ο server.


Συγχρονισμός:

worker thread: 
Κάθε worker thread περιέχει ένα while loop το οποίο τρέχει μόνο αν η global μεταβλητή stop_worker_threads είναι false, με λίγα λόγια αν ο χρήστης δώσει το exit command στον server τότε η μεταβλητή αυτή γίνεται true και τα worker threads βγάινουν από τη λούπα. Με αυτόν τον τρόπο κάθε worker thread μπορεί να εκτελέσει πολλές δουλείες. Ο συγχρονισμός γίνεται ως εξής: αρχικά κλειδώνεται το mutex που προστατεύει τον κοινό buffer και στη συνέχεια ένα condition variable με όνομα nonempty κάνει pthread_cond_wait μέσα σε ένα while loop το οποίο βγαίνει απ το loop μόνο όταν τηρέι τα παρακάτω conditions. Αν ο buffer είναι άδειος ή αν το concurrency level είναι μεγαλύτερο ή ίσο με το max_concurrency και αν το stop_worker_threads flag έιναι false. Δηλαδή κάθε worker thread περιμένει (γίνεται suspend λόγω της pthread_cond_wait χωρίς busy waiting) μέχρι ο buffer να αποκτήσει τουλάχιστον ένα job και το concurrency level να είναι μικρότερο του max concurrency (δηλαδή αν εκτελούνται λιγότερες εργασίες από το max_concurrency) ή αν ο χρήστης κάνει exit τον server. Τέλος κλειδώνει το buffer mutex.

controller thread: 
Κάθε controller thread αρχικά κάνει lock το mutex για τον buffer και στη συνέχεια μέσα σε ένα while loop κάνει pthread_cond_wait σε ένα condition variable με όνομα nonfull μέχρι να αδειάσει τουλάχιστον μια θέση στον buffer ώστε να μπορέσει να κάνει insert το καινούριο job.
Δηλαδή περιμένει όσο ο buffer είναι γεμάτος. Μόλις εισάγει μια δουλειά στον buffer τότε στέλνει σήμα (με την χρήση pthread_cond_broadcast) σε όλα τα worker threads που περιμένουν με pthread_cond_wait στη nonempty οτι υπάρχει μια δουλειά στο buffer. Αντίστοιχα τα worker threads μόλις βγάλουν μια δουλειά από τον buffer στέλνουν σήμα στην σε όλα τα controller threads που περιμένουν στη nonfull ώστε να ενημερωθούν οτι υπάρχει χώρος στον buffer να εισάγουν καινούρια δουλειά. Τέλος κλειδώνει το buffer mutex.

Με τον παραπάνω τρόπο επιτυγχάνεται σωστή λειτουργία multiple consumers - multiple producers σε ένα κοινό buffer.

Επίσης όταν αυξάνεται το concurrency από ένα setConcurrency command τότε στέλνεται σήμα σε όλους τους worker threads που περιμένουν condition variable nonempty ώστε να μπορούν να εκτελέσουν και άλλες εργασίες που βρίσκονται στον buffer (αν προφανώς υπάρχουν στον buffer).
Mε αυτόν τον τρόπο "ξυπνάνε" τα worker threads όταν αλλάζει το concurrency.


main thread:
Το main thread αρχικά κάνει intialize όλα τα shared δεδομένα (buffer, mutex, cond vars κλπ) και το server socket και δημιουργεί και τρέχει τα worker threads. Στη συνέχεια ξεκινάει το server loop (while loop), το οποίο τρέχει μέχρι ο χρήστης να δώσει την εντολή exit. Ξεκινάει να δέχεται connections στο socket από τους jobCommanders διαβάζει τα δεδομένα που έστειλε ο jobCommander και δημιουργεί και τρέχει ένα controller thread, για να διαχειριστεί ανάλογα το command που έστειλε ο jobCommander. Τέλος περιμένει να τελειώσουν όλες οι εργασίες που εκτελούνται και για κάθε διεργασία που έχει απομέινει στον buffer και δεν έχει εκτελεστεί , στέλνει μήνυμα στον jobCommander που κατάθεσε την εργασία οτί δεν εκτελέστηκε.



Makefile:

make all: μεταγλωτίζει τον κώδικα και παράγει τα εκτελέσιμα αρχεία.
make clean: διαγράφει τα εκτελέσιμα αρχεία.