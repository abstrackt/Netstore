## Krótka instrukcja użycia

Aby korzystać z Netstore należy skompilować pliki źródłowe za pomocą dostarczonego makefile, po czym aktywować węzeł serwerowy za pomocą polecenia

    ./netstore-server --addr {adres} --file {folder wyjściowy} --port {numer portu} (oraz opcjonalnie) --buff {rozmiar bufora węzła} --time {czas oczekiwania na odpowiedź} 

Następnie węzeł kliencki uruchamiamy poleceniem

    ./netstore-client --addr {adres} --out {folder wyjściowy} --port {numer portu} --time {czas oczekiwania na odpowiedź}

## Wprowadzenie

Zadanie polega na napisaniu sieciowej aplikacji rozproszonego przechowywania plików. Aplikacja składa się z węzłów serwerowych [zwanych w treści zadania także wymiennie serwerami] i węzłów klienckich [zwanych w treści zadania także wymiennie klientami]. Węzły serwerowe i klienckie komunikują się między sobą poprzez sieć zgodnie ze zdefiniowanym dalej protokołem. Węzły współpracują ze sobą, tworząc grupę węzłów. Grupa może składać się z dowolnej liczby węzłów. Węzły mogą dynamicznie dołączać do grupy lub odłączać się od grupy. Każdy węzeł udostępnia ten sam zestaw funkcjonalności danego typu (serwerowy lub kliencki) i wszystkie węzły dowolnego typu są sobie równe pod względem praw, priorytetów i możliwości w utworzonej grupie węzłów. Węzły klienckie dostarczają interfejs użytkownika pozwalający przede wszystkim na wysyłanie nowych plików do przechowywania w grupie, usuwanie ich lub pobieranie i poszukiwanie plików przechowywanych w grupie. Węzły serwerowe natomiast mają za zadanie przechowywać pliki.

## Skrócona funkcjonalność aplikacji

*   Każdy węzeł serwerowy udostępnia pewną zadaną przestrzeń pamięci nieulotnej.

*   Węzły tworzą grupę dzięki wykorzystaniu tego samego adresu IP Multicast.

*   Węzły serwerowe w jednej grupie umożliwiają przechowywanie dowolnej liczby plików o łącznym rozmiarze nieprzekraczającym łącznej przestrzeni dyskowej udostępnianej przez wszystkie węzły serwerowe w danej grupie.

*   Łączna przestrzeń do przechowywania plików zmienia się dynamicznie wraz z dołączaniem i odłączaniem się węzłów serwerowych w grupie.

*   Pliki przechowywane przez dowolny węzeł serwerowy widoczne są przez wszystkie węzły klienckie w danej grupie.

*   Pliki przechowywane są niepodzielnie: jeden plik w całości przechowywany jest w pamięci nieulotnej węzła serwerowego.

*   Pliki identyfikowane są po nazwie, a wielkość liter ma znaczenie.

*   Każdy węzeł kliencki umożliwia dodanie nowego pliku do plików przechowywanych przez grupę lub usunięcie dowolnego istniejącego pliku.

*   Każdy węzeł kliencki umożliwia pobranie zawartości dowolnego pliku z obecnie przechowywanych plików w grupie węzłów serwerowych

*   Każdy węzeł kliencki umożliwia pobranie listy plików obecnie przechowywanych w grupie węzłów serwerowych.

## Protokół komunikacji między węzłami

##### Format komunikatów

Węzły do komunikacji między sobą używają protokołu UDP. Pakiety protokołu komunikacyjnego wymieniane pomiędzy węzłami mają postać:

    SIMPL_CMD

lub  

    CMPLX_CMD    char[10] cmd    uint64 cmd_seq    uint64 param    char[] data

<div>Wartości w polach `param` oraz `cmd_seq` powinny być przesyłane w sieciowej kolejności bajtów (ang. _big endian_).</div>

W przypadku potrzeby przesłania wartości zmiennej o zadeklarowanej liczbie bitów mniejszej od 64, aplikacja powinna w pierwszej kolejności zrzutować wartość tej zmiennej na typ 64-bitowy a następnie przed wysłaniem wartości przez sieć odpowiednio przygotować kolejność bajtów przy użyciu funkcji `htobe64()`.

Jeśli wartość przesyłana w polu `cmd` jest krótsza niż 10 znaków, to zawartość pola `cmd` powinna zostać uzupełniona zerami.

##### W dalszej części specyfikacji używane są następujące oznaczenia:

*   MCAST_ADDR – adres IP rozgłaszania (ang. _multicast_) ukierunkowanego używany przez wszystkie węzły jednej grupy;

*   CMD_PORT – numer portu UDP, na którym węzły serwerowe danej grupy nasłuchują pakietów.

Każdy z węzłów serwerowych powinien nasłuchiwać pakietów UDP na porcie CMD_PORT pod adresem rozgłoszeniowym MCAST_ADDR oraz swoim adresem jednostkowym (ang. _unicast_).

Wszystkie rozmiary plików i przestrzeni dyskowej wyrażane są w liczbie bajtów.

Serwer poznaje adres jednostkowy klienta na podstawie otrzymanego datagramu protokołu UDP.

#### Parowanie odpowiedzi serwera z zapytaniami klienta

Klient przy wysyłaniu każdego zapytania do serwera powinien wypełnić zawartość pola `cmd_seq` unikalną wartością numeryczną umożliwiającą klientowi rozpoznawanie odpowiedzi serwera lub serwerów. Jeśli klient w trakcie swojego działania otrzyma pakiet danych, w którym wartość pola `cmd_seq` nie pokrywa się z wartością wysłaną i oczekiwaną w odpowiedzi, to klient powinien wypisać na standardowy strumień błędów jednoliniowy komunikat błędu o formacie określonym dalej w treści zadania.

Serwer w odpowiedzi powinien zawsze przepisać wartość pola `cmd_seq` z otrzymanego pakietu do pola o takiej samej nazwie w pakiecie z odpowiedzią.

Dalej w treści zadania pole `cmd_seq` nie będzie wspominane dla lepszej czytelności i przejrzystości, ale jest ono obligatoryjne we wszystkich wysyłanych pakietach.

#### Rozpoznawanie listy serwerów w grupie

W celu poznania listy aktywnych węzłów serwerowych w grupie klient wysyła na adres rozgłoszeniowy MCAST_ADDR i port CMD_PORT pakiet `SIMPL_CMD` z poleceniem `cmd = “HELLO”` oraz pustą zawartością pola `data`.

Każdy z węzłów serwerowych po otrzymaniu powyższego pakietu powinien odpowiedzieć do nadawcy bezpośrednio na jego adres jednostkowy i port nadawcy pakietem `CMPLX_CMD` zawierającym:

*   pole `cmd` z wartością `“GOOD_DAY”`;

*   pole `param` z wartością wolnej przestrzeni dyskowej na przechowywanie plików;

*   pole `data` z wartością tekstową zawierającą adres MCAST_ADDR, na jakim serwer nasłuchuje, w notacji kropkowej.

##### Przeglądanie listy plików i wyszukiwanie na serwerach w grupie

W celu poznania listy wszystkich plików aktualnie przechowywanych w węzłach danej grupy klient wysyła na adres rozgłoszeniowy MCAST_ADDR i port CMD_PORT pakiet `SIMPL_CMD` z poleceniem `cmd = “LIST”` i pustą wartością pola `data`.

W celu odnalezienia w grupie plików zawierających w nazwie zadany ciąg znaków klient wysyła na adres rozgłoszeniowy MCAST_ADDR i port CMD_PORT pakiet `SIMPL_CMD` z poleceniem `cmd = “LIST”` oraz szukanym ciągiem znaków w polu `data`.

Każdy węzeł serwerowy po otrzymaniu jednego z powyższych pakietów powinien odpowiedzieć nadawcy bezpośrednio na jego adres jednostkowy i port nadawcy pakietem `SIMPL_CMD`:

*   Pole `cmd` z wartością `“MY_LIST”`.

*   Pole `data` powinno zawierać listę nazw plików przechowywanych przez dany węzeł; nazwy plików powinien rozdzielać znak nowego wiersza (znakiem nowego wiersza jest `‘\n’`). Jeśli lista wszystkich nazw plików danego serwera spowodowałaby przekroczenie dopuszczalnego rozmiaru jednego pakietu danych UDP, to serwer powinien podzielić listę nazw swoich plików na odpowiednią liczbę pakietów, wysyłając wielokrotnie odpowiedź `“MY_LIST”` z kolejnymi nazwami plików, aż do wysłania całej listy. Zakładamy, że nie obsługujemy plików o nazwach przekraczających dopuszczalny rozmiar danych jednego pakietu UDP.

Jeśli otrzymany przez serwer pakiet z `cmd = “LIST”` zawiera niepusty ciąg znaków w polu `data`, to serwer powinien w odpowiedzi przesłać tylko te nazwy plików, które zawierają ów niepusty ciąg znaków (wielkość liter ma znaczenia przy porównywaniu nazw plików). Jeśli żadna nazwa pliku nie zawiera poszukiwanego ciągu znaków, to serwer nie odpowiada żadnym pakietem do nadawcy na otrzymane zapytanie. Reakcja serwera będzie taka sama, jeśli nie przechowuje obecnie żadnego pliku.

##### Pobieranie pliku z serwera

Dowolny węzeł kliencki ma prawo pobrać dowolny plik z dowolnego węzła serwerowego w danej grupie. Klient w celu rozpoczęcia pobierania konkretnego pliku wysyła do wybranego węzła serwerowego na jego adres jednostkowy i port CMD_PORT pakiet `SIMPL_CMD` z poleceniem `cmd = “GET”` oraz nazwą pliku w polu `data`.

Serwer po otrzymaniu powyższego komunikatu odpowiada nadawcy na jego adres jednostkowy i port nadawczy pakietem `CMPLX_CMD` zawierającymi:

*   pole `cmd = “CONNECT_ME”`;

*   pole `param` z numerem portu TCP, na którym oczekuje połączenia od klienta;

*   pole `data` z nazwą pliku, który zostanie wysłany.

Jeśli serwer otrzyma pakiet z żądaniem pobrania pliku, którego serwer nie ma, to serwer nie odpowiada na taki pakiet, ale powinien odnotować otrzymanie niewłaściwego pakietu zgodnie ze specyfikacją opisaną w dalszej treści.

Klient po otrzymaniu powyższego komunikatu od serwera powinien nawiązać połączenie TCP z węzłem serwerowym, używając jego adresu jednostkowego oraz otrzymanego portu TCP. Serwer po nawiązaniu połączenia z klientem wysyła zawartość pliku i kończy połączenie.

##### Usuwanie pliku z serwera

Dowolny węzeł kliencki ma prawo skasować dowolny plik z dowolnego węzła serwerowego w danej grupie. Klient w celu skasowania danego pliku z grupy wysyła na adres rozgłoszeniowy MCAST_ADDR (dozwolone jest wysłanie także na adres unicast wybranego serwera) i port CMD_PORT pakiet `SIMPL_CMD` z poleceniem `cmd = “DEL”` oraz z nazwą pliku do skasowania w polu `data`.

Każdy węzeł serwerowy po otrzymaniu powyższego komunikatu usuwa trwale plik wskazany nazwą, jeśli taki przechowuje.

##### Dodawanie pliku do grupy

Klient w celu wysłania pliku do przechowywania go w grupie musi najpierw wyznaczyć węzeł, który będzie przechowywał ten plik. W tym celu może wykorzystać polecenie służące rozpoznawaniu serwerów w grupie opisane wcześniej (patrz opis dla `cmd = “HELLO”`).

Po wybraniu serwera klient komunikuje się z nim, wysyłając na jego adres jednostkowy oraz port CMD_PORT pakiet `CMPLX_CMD` zawierający:

*   pole `cmd = “ADD”`;

*   pole `param` z rozmiarem pliku;

*   pole `data` z nazwą pliku, który zostanie wysłany.

Węzeł serwerowy po otrzymaniu powyższego komunikatu musi odpowiedzieć klientowi w zależności od aktualnej możliwości przyjęcia pliku. Jeśli serwer nie ma już miejsca na przechowanie pliku o rozmiarze wskazanym przez klienta, to odpowiada klientowi, wysyłając na jego adres jednostkowy i port nadawcy pakiet `SIMPL_CMD` z `cmd = “NO_WAY”` oraz nazwą pliku, jaki klient chciał dodać w polu `data`. Jeśli serwer przechowuje już obecnie plik o wskazanej przez węzeł kliencki nazwie, to serwer także powinien odpowiedzieć klientowi, wysyłając na jego adres jednostkowy i port nadawcy pakiet `SIMPL_CMD` z `cmd = “NO_WAY”` oraz nazwą pliku, jaki klient chciał dodać w polu `data`. Serwer powinien odmówić przyjęcia pliku także w przypadku gdy otrzymana nazwa pliku jest pusta lub zawiera znak `'/’`, wysyłając na adres jednostkowy i port nadawcy pakiet `SIMPL_CMD` z `cmd = “NO_WAY”` oraz nazwą pliku, jaki klient chciał dodać, w polu `data`.

Jeśli natomiast serwer dysponuje obecnie wolnym miejscem pozwalającym przechować plik o rozmiarze wskazanym przez nadawcę, to odpowiada klientowi na jego adres jednostkowy i port nadawcy pakietem `CMPLX_CMD` zawierającym:

*   pole `cmd` z wartością `“CAN_ADD”`;

*   pole `param` z numerem portu TCP, na którym serwer oczekuje połączenia od klienta;

*   pole `data` pozostaje puste.

Klient po otrzymaniu powyższego komunikatu powinien nawiązać połączenie TCP z węzłem serwerowym, używając jego adresu jednostkowego oraz otrzymanego portu TCP. Klient wysyła zawartość pliku, po czym powinien zakończyć połączenie.

## Część A - Węzeł serwerowy

Zadanie polega na napisaniu programu implementującego zachowanie węzła serwerowego. Program powinien przyjmować następujące parametry linii poleceń:

*   MCAST_ADDR – adres rozgłaszania ukierunkowanego, ustawiany obowiązkowym parametrem `-g` węzła serwerowego;

*   CMD_PORT – port UDP używany do przesyłania i odbierania poleceń, ustawiany obowiązkowym parametrem `-p` węzła serwerowego;

*   MAX_SPACE – maksymalna liczba bajtów udostępnianej przestrzeni dyskowej na pliki grupy przez ten węzeł serwerowy, ustawiana opcjonalnym parametrem `-b` węzła serwerowego, wartość domyślna 52428800;

*   SHRD_FLDR – ścieżka do dedykowanego folderu dyskowego, gdzie mają być przechowywane pliki, ustawiany parametrem obowiązkowym `-f` węzła serwerowego;

*   TIMEOUT – liczba sekund, jakie serwer może maksymalnie oczekiwać na połączenia od klientów, ustawiane opcjonalnym parametrem `-t` węzła serwerowego, wartość domyślna 5, wartość maksymalna 300.

Serwer podczas uruchomienia powinien zindeksować wszystkie pliki znajdujące się bezpośrednio w folderze SHRD_FLDR, a ich łączny rozmiar liczony w bajtach odjąć od parametru MAX_SPACE. Serwer nie indeksuje plików w podkatalogach folderu SHRD_FLDR.

Serwer powinien podłączyć się do grupy rozgłaszania ukierunkowanego pod wskazanym adresem MCAST_ADDR. Serwer powinien nasłuchiwać na porcie CMD_PORT poleceń otrzymanych z sieci protokołem UDP także na swoim adresie unicast. Serwer powinien reagować na pakiety UDP zgodnie z protokołem opisanym wcześniej.

Jeśli serwer otrzyma polecenie dodania pliku lub pobrania pliku, to powinien otworzyć nowe gniazdo TCP na losowym wolnym porcie przydzielonym przez system operacyjny i port ten przekazać w odpowiedzi węzłowi klienckiemu. Serwer oczekuje maksymalnie TIMEOUT sekund na nawiązanie połączenia przez klienta i jeśli takie nie nastąpi, to port TCP powinien zostać niezwłocznie zamknięty. Serwer w czasie oczekiwania na podłączenie się klienta i podczas przesyłania pliku powinien obsługiwać także inne zapytania od klientów.

Jeśli serwer otrzyma polecenia dodania pliku, to odpowiedź klientowi pakietem `“CAN_ADD”` oznacza jednocześnie zarezerwowanie miejsca na serwerze niezbędnego do przyjęcia pliku od klienta.

Jakiekolwiek pakiety otrzymane przez program niezgodne ze specyfikacją protokołu powinny być pomijane przez serwer. Informacja o niewłaściwym pakiecie powinna zostać wypisana na standardowe wyjście błędów,  jednolinijkowy komunikat o błędzie dla każdego niepoprawnego pakietu. Informacja o błędzie powinna zawierać informację w formacie:

    [PCKG ERROR] Skipping invalid package from {IP_NADAWCY}:{PORT_NADAWCY}.

gdzie `{IP_NADAWCY}` jest adresem IP nadawcy otrzymanego datagramu UDP  
a `{PORT_NADAWCY}` jest numerem portu nadawcy datagramu UDP

Autor programu powinien uzupełnić wiadomość po kropce o dodatkowe informacje opisujące błąd, ale bez użycia znaku nowej linii.

Serwer powinien zakończyć swoje działanie łagodnie, to znaczy kończąc otwarte połączenia oraz zwalniając pobrane zasoby systemowe, po otrzymaniu sygnału `CTRL+C`.

## Część B - Węzeł kliencki

Zadanie polega na napisaniu programu implementującego zachowanie węzła klienckiego. Program powinien przyjmować następujące parametry linii poleceń:

*   MCAST_ADDR – adres rozgłaszania ukierunkowanego (może być także adresem broadcast), ustawiany obowiązkowym parametrem `-g`; klient powinien używać tego adresu do wysyłania komunikatów do grupy węzłów serwerowych;

*   CMD_PORT – port UDP, na którym nasłuchują węzły serwerowe, ustawiany obowiązkowym parametrem `-p`; klient powinien używać tego numeru portu, aby wysyłać komunikaty do węzłów serwerowych;

*   OUT_FLDR – ścieżka do folderu dyskowego, gdzie klient będzie zapisywał pobrane pliki, ustawiany parametrem obowiązkowym `-o`;

*   TIMEOUT – czas oczekiwania na zbieranie informacji od węzłów wyrażony w sekundach; akceptowana wartość powinna być dodatnia i większa od zera; wartość domyślna 5; wartość maksymalna 300; może zostać zmieniona przez opcjonalny parametr `-t`.

Klient po rozpoczęciu swojej pracy powinien oczekiwać na polecenia użytkownika na standardowym wejściu. Każde polecenie kończy się znakiem nowej linii `'\n'`. Rozpoznawane przez program kliencki polecenia użytkownika (wielkość liter nie ma znaczenia):

*   `discover` – po otrzymaniu tego polecenia klient powinien wypisać na standardowe wyjście listę wszystkich węzłów serwerowych dostępnych aktualnie w grupie. Klient oczekuje na zgłoszenia serwerów przez TIMEOUT sekund, w trakcie oczekiwania interfejs użytkownika zostaje wstrzymany. Dla każdego odnalezionego serwera klient powinien wypisać na standardowe wyjście w jednej linii adres jednostkowy IP tego serwera, następnie w nawiasie adres MCAST_ADDR otrzymany od danego serwera, a na końcu rozmiar dostępnej przestrzeni dyskowej na tym serwerze. Oczekiwany format takiej linii:

    Found 10.1.1.28 (239.10.11.12) with free space 23456

*   `search %s` – klient powinien uznać polecenie za prawidłowe, także jeśli podany ciąg znaków `%s` jest pusty. Po otrzymaniu tego polecenia klient wysyła po sieci do węzłów serwerowych zapytanie w celu wyszukania plików zawierających ciąg znaków podany przez użytkownika (lub wszystkich plików jeśli ciąg znaków `%s` jest pusty), a następnie przez TIMEOUT sekund nasłuchuje odpowiedzi od węzłów serwerowych. Otrzymane listy plików powinny zostać wypisane na standardowe wyjście po jednej linii na jeden plik. Każda linia powinna zawierać informację:  

        {nazwa_pliku} ({ip_serwera})

    gdzie:

        `{nazwa_pliku}` to nazwa pliku otrzymana z serwera;

        `{ip_serwera}` to adres jednostkowy IP serwera, z którego dana nazwa pliku została przesłana.

    Pakiety z odpowiedziami od serwerów z listą plików otrzymane po upływie TIMEOUT powinny zostać zignorowane przez klienta. Interfejs użytkownika zostaje wstrzymany na czas oczekiwania odpowiedzi z serwerów.

*   `fetch %s` – użytkownik może wskazać nazwę pliku `%s`, tylko jeśli nazwa pliku występowała na liście otrzymanej w wyniku ostatniego wykonania polecenia search. W przeciwnym przypadku klient nie podejmuje akcji pobierania pliku, jednocześnie informując użytkownika o błędzie jednoliniowym komunikatem na standardowe wyjście. Jeśli wskazany plik istnieje w ostatnio wyszukiwanych, to klient powinien wybrać dowolny węzeł serwerowy, który przechowuje plik dokładnie wskazany przez podaną nazwę pliku `%s` i rozpocząć pobieranie pliku, zapisując plik do folderu OUT_FLDR. W trakcie pobierania pliku użytkownik powinien móc kontynuować korzystanie z programu. Po zakończeniu pobierania pliku klient powinien wypisać na standardowe wyjście komunikat o zakończeniu pobierania pliku w formacie:  

        File {%s} downloaded ({ip_serwera}:{port_serwera})

    gdzie:

        `{%s}` to nazwa pliku;

        `{ip_serwera}` to adres IP unicast serwera;

        `{port_serwera}` to port TCP serwera użyty do pobrania pliku.

    Jeśli pobieranie pliku nie powiedzie się, to klient powinien wypisać na standardowe wyjście komunikat o błędzie w formacie:

        File {%s} downloading failed ({ip_serwera}:{port_serwera}) {opis_błędu}

    gdzie:

        `{%s}` to nazwa pliku;

        `{ip_serwera}` to adres jednostkowy IP serwera;

        `{port_serwera}` to port TCP serwera użyty do pobrania pliku;

        `{opis_błędu}` to komunikat słowny opisujący przyczynę błędu.

*   `upload %s` – użytkownik powinien wskazać ścieżkę do pliku, który chce wysłać do przechowania w grupie. Użytkownik może podać pełną ścieżkę bezwzględną do pliku bądź ścieżkę względną. Jeśli użytkownik wskaże nazwę pliku poprzez ścieżkę względną, to należy rozpocząć szukanie pliku do wysłania w katalogu bieżącym. Jeśli wskazany plik nie istnieje, to klient powinien poinformować o tym fakcie użytkownika jednoliniową informacją o błędzie na standardowe wyjście w formacie  

        File {%s} does not exist

    gdzie:

            `{%s}` to nazwa pliku.  

    Klient powinien podjąć próbę wysłania pliku w pierwszej kolejności do węzła serwerowego o największej dostępnej wolnej przestrzeni. Jeśli żaden węzeł nie dysponuje wystarczającym wolnym miejscem, to klient powinien poinformować użytkownika o braku możliwości załadowania pliku, wypisując jednoliniową informację o błędzie na standardowe wyjście w formacie:  

        File {%s} too big

        gdzie:

            `{%s}` to nazwa pliku.

    W trakcie wysyłania pliku użytkownik powinien móc w dalszym ciągu korzystać z aplikacji. Po zakończeniu wysyłania pliku klient powinien wypisać na standardowe wyjście komunikat o zakończeniu wysyłania pliku w formacie:

        File {%s} uploaded ({ip_serwera}:{port_serwera})

    gdzie:

        `{%s}` to nazwa pliku;

        `{ip_serwera}` to adres jednostkowy serwera;

        `{port_serwera}` to port TCP serwera użyty do pobrania pliku.

    Jeśli wysyłanie pliku nie powiedzie się, to klient powinien wypisać na standardowe wyjście komunikat o błędzie w fomacie:

         File {%s} uploading failed ({ip_serwera}:{port_serwera}) {opis_błędu}

    gdzie:

        `{%s}` to nazwa pliku;

        `{ip_serwera}` to adres jednostkowy serwera

        `{port_serwera}` to port TCP serwera użyty do pobrania pliku;

        `{opis_błędu}` to komunikat słowny opisujący przyczynę błędu.

*   `remove %s` – klient po otrzymaniu tego polecenia powinien wysłać do grupy serwerów zlecenie usunięcia wskazanego przez użytkownika pliku. Polecenie jest prawidłowe, tylko jeśli podana nazwa pliku `%s` jest niepusta.

*   `exit` – klient po otrzymaniu tego polecenia powinien zakończyć wszystkie otwarte połączenia i zwolnić pobrane zasoby z systemu oraz zakończyć pracę aplikacji.

Jakiekolwiek inne polecenia wpisane przez użytkownika powinny zostać uznane jako nieprawidłowe i zignorowane.

Jeśli klient w trakcie działania otrzyma pakiet sieciowy nie wynikający z opisanego protokołu (w szczególności także pakiet o nieoczekiwanej wartości pola `cmd_seq`), to powinien wypisać jednoliniową informację na standardowy strumień błędów o takiej samej postaci jak w przypadku serwera.  

    [PCKG ERROR]  Skipping invalid package from {IP_NADAWCY}:{PORT_NADAWCY}.

gdzie `{IP_NADAWCY}` jest adresem IP nadawcy otrzymanego datagramu UDP  
a `{PORT_NADAWCY}` jest numerem portu nadawcy datagramu UDP

Autor programu powinien uzupełnić wiadomość po kropce o dodatkowe informacje opisujące błąd, ale bez użycia znaku nowej linii.
