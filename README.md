# ZOS Dokumentace

	Jméno: Jakub Šlechta
	o.č.: A20B0243P
	KIV/ZOS 2022/2023

---

## Zadání

Tématem semestrální práce bude práce se zjednodušeným souborovým systémem založeným na pseudoFAT. Vaším cílem bude splnit několik vybraných úloh. 

Základní funkčnost, kterou musí program splňovat. Formát výpisů je závazný. 

Program bude mít jeden parametr a tím bude název Vašeho souborového systému. Po spuštění bude program čekat na zadání jednotlivých příkazů s minimální funkčností, viz. [ZOS2022_SP](https://courseware.zcu.cz/CoursewarePortlets2/DownloadDokumentu?id=219737).

---

## Souborový systém FAT

![FAT FS](https://www.kencorner.com/wp-content/uploads/2018/03/FATFileSystem.png)

### Boot sektor (Reserved area)

Sektor na počáteční adrese. Jedná se o logický oddíl obsahující metadata důležitá pro přístupu k souborovému systému. Obsahuje informace jako např. velikost clusterů, velikost sekce, signaturu, atd.

### File allocation table 

File allocation table, neboli FAT, slouží k mapovaní souborů na jednotlivé datové oblasti (clustery). Soubory poté tvoří jednosměrný řetěz zakončený unikátním znakem označující konec dat souboru. Obvykle existují dvě kopie. Ke specíalním znakům ještě patří znak označující volný cluster a vadný cluster.

### Datový oddíl

Datový oddíl je členěn do clusterů o stejné velikosti a obsahuje soubory a adresáře. Jejich stav/přiřazení je dále popsán právě ve FAT. 

Soubory obsahují pouze svá data.

Adresáře se skládají ze záznamů o stejné velikosti, které z pravidla popisují cílový cluster souboru/podadresáře, zda se jedná o soubor/podadresář, název záznamu a v případě souboru jeho velikost.

#### Kořenový adresář

Adresář na počáteční adrese datového oddílu.

---

## Popis zavedených omezení

- V řešení nám bude stačit jedna FAT tabulka, ale mějte na paměti, že reálný fs má typicky dvě FAT tabulky.
- Předpokládáme, že adresář se vždy vejde do jednoho clusteru (limituje nám počet záznamů v adresáři).
- Maximální délka názvu souboru bude 8+3=11 znaků (jméno.přípona) + `\0` (ukončovací znak v C/C++), tedy 12 bytů.
- Každý název bude zabírat právě 12 bytů (do délky 12 bytů doplníte `\0` - při kratších názvech).


---
## Struktura a řešení aplikace

	Aplikace je řešena v c++.

### ICommand

Jednotné rozhraní pro všechny příkazy fs.

Rozhraní je poměrně jednoduché:
```
class ICommand {  
private:  
    virtual bool validateArguments() = 0;  
  
    virtual bool run() = 0;  
  
protected:  
    std::shared_ptr<FileSystem> mFS;  
    int mOptCount;  
    std::string mOpt1;  
    std::string mOpt2;  
  
public:  
    virtual ~ICommand() = default;  
  
    void process();  
  
    explicit ICommand(const std::vector<std::string> &options);  
  
    ICommand &registerFS(const std::shared_ptr<FileSystem> &pFS);  
};
```

Každý příkaz má 0 až 2 vstupních parametrů. Při vytvoření příkazu se inicializuje počet přijatých příkazů do `mOptCount`.

Jednotlivé argumenty jsou uloženy do proměnných `mOpt1` a `mOpt2`.

Jako poslední má každý příkaz přístup k danému fs pomocí ukazatele `mFS`.

### Commands

Implementace jednotlivých příkazů.

Implementované příkazy:

```
enum class ECommands {  
    eCpCommand,  
    eMvCommand,  
    eRmCommand,  
    eMkdirCommand,  
    eRmdirCommand,  
    eLsCommand,  
    eCatCommand,  
    eCdCommand,  
    ePwdCommand,  
    eInfoCommand,  
    eIncpCommand,  
    eOutcpCommand,  
    eLoadCommand,  
    eFormatCommand,  
    eDefragCommand,  
    // Classless commands  
    eExitCommand,  
    eUnknownCommand,  
};
```


A jejich volání z aplikační konzole: 

```
constexpr ECommands getCommandCode(std::string const &string) {  
    if (string == "cp") return ECommands::eCpCommand;  
    if (string == "mv") return ECommands::eMvCommand;  
    if (string == "rm") return ECommands::eRmCommand;  
    if (string == "mkdir") return ECommands::eMkdirCommand;  
    if (string == "rmdir") return ECommands::eRmdirCommand;  
    if (string == "ls") return ECommands::eLsCommand;  
    if (string == "cat") return ECommands::eCatCommand;  
    if (string == "cd") return ECommands::eCdCommand;  
    if (string == "pwd") return ECommands::ePwdCommand;  
    if (string == "info") return ECommands::eInfoCommand;  
    if (string == "incp") return ECommands::eIncpCommand;  
    if (string == "outcp") return ECommands::eOutcpCommand;  
    if (string == "load") return ECommands::eLoadCommand;  
    if (string == "format") return ECommands::eFormatCommand;  
    if (string == "defrag") return ECommands::eDefragCommand;  
    if (string == "exit") return ECommands::eExitCommand;  
    return ECommands::eUnknownCommand;  
}
```

kde `string` je uživatelský vstup. Popis příkazů viz. zadání.

### FileSystem

Rozhraní se základními operacemi nad fs. Mělo by se jednat o sdílenou implementaci všech příkazů. Patří sem např. získání záznamu z relativní cesty, smazání záznamu, zapsaní souborového řetězu do FAT, získání FAT řetězce, atd.

Jednotlivé prvky fs:
- BootSector
- FAT
- DirectoryEntry
	- Záznam souboru či adresáře.

### Ostatní soubory
- main
	- vstupní bod aplikace.
- utils/*
	- funkce pro práci s řetězci, streamy, serializace, validátory ...
- definitions
	- definice fs, zpráv výjimek, specifikačních konstant...

---

## Spuštění a běh aplikace

### Spuštění

`<executable> <fs_name>`

např.:

`./zos_sp FS_A20B0243P.bin`

### Běh aplikace

Jedná se o konzolovu aplikaci. Po spuštění se zobrazí:

```
(... informace o fs ...)

/ $ 
```

a aplikace bude čekat na uživatelský vstup.

Pokud budeme např. chtít vytvořit nový adresář, použijeme příkaz `mkdir`:
```
/ $ mkdir new_dir
OK
/ $ 
```

Zda se adresář opravdu vytvořil můžeme zkontrolovat např. přepnutím do nově vzniklého adresáře příkazem `cd`:
```
/ $ cd new_dir
OK
/new_dir $  
```

---

## Závěr

Při psaní aplikace jsem nenarazil na žádné vetší potíže a práce na semestrálce mě i celkem bavila. Navíc jsem měl příležitost vyzkoušet si můj nový oblíbený jazyk a podpořit tak poznatky nabrané z KIV/CPP. 