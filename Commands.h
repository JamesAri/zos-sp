#ifndef ZOS_SP_COMMANDS_H
#define ZOS_SP_COMMANDS_H

#include "ICommand.h"
#include "FileSystem.h"

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
    // commands with no class
    eExitCommand,
    eUnknownCommand,
};

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

/**
Zkopíruje soubor s1 do umístění s2
cp s1 s2
Možný výsledek:
OK
FILE NOT FOUND (není zdroj)
PATH NOT FOUND (neexistuje cílová cesta)
 */
class CpCommand : public ICommand {
public:
    using ICommand::ICommand;
    using ICommand::process;
    using ICommand::registerFS;

private:
    bool validate_arguments() override;

    bool run() override;
};

/**
Přesune soubor s1 do umístění s2, nebo přejmenuje s1 na s2
mv s1 s2
Možný výsledek:
OK
FILE NOT FOUND (není zdroj)
PATH NOT FOUND (neexistuje cílová cesta)
 */
class MvCommand : public ICommand {

public:
    using ICommand::ICommand;
    using ICommand::process;
    using ICommand::registerFS;

private:
    bool validate_arguments() override;

    bool run() override;
};

/**
rm s1
Možný výsledek:
OK
FILE NOT FOUND
 */
class RmCommand : public ICommand {

public:
    using ICommand::ICommand;
    using ICommand::process;
    using ICommand::registerFS;

private:
    bool validate_arguments() override;

    bool run() override;
};

/**
Vytvoří adresář a1
mkdir a1
Možný výsledek:
OK
PATH NOT FOUND (neexistuje zadaná cesta)
EXIST (nelze založit, již existuje)
 */
class MkdirCommand : public ICommand {

public:
    using ICommand::ICommand;
    using ICommand::process;
    using ICommand::registerFS;

private:
    std::vector<std::string> mAccumulator;

    bool validate_arguments() override;

    bool run() override;
};

/**
Smaže prázdný adresář a1
rmdir a1
Možný výsledek:
OK
FILE NOT FOUND (neexistující adresář)
NOT EMPTY (adresář obsahuje podadresáře, nebo soubory)
 */
class RmdirCommand : public ICommand {

public:
    using ICommand::ICommand;
    using ICommand::process;
    using ICommand::registerFS;

private:
    bool validate_arguments() override;

    bool run() override;
};

/**
Vypíše obsah adresáře a1, bez parametru vypíše obsah aktuálního adresáře
ls a1
ls
Možný výsledek:
FILE: f1
DIR: a2
PATH NOT FOUND (neexistující adresář)
 */
class LsCommand : public ICommand {

public:
    using ICommand::ICommand;
    using ICommand::process;
    using ICommand::registerFS;

private:
    std::vector<std::string> mAccumulator;

    bool validate_arguments() override;

    bool run() override;
};

/**
Vypíše obsah souboru s1
cat s1
Možný výsledek:
OBSAH
FILE NOT FOUND (není zdroj)
 */
class CatCommand : public ICommand {

public:
    using ICommand::ICommand;
    using ICommand::process;
    using ICommand::registerFS;

private:
    bool validate_arguments() override;

    bool run() override;
};

/**
Změní aktuální cestu do adresáře a1
cd a1
Možný výsledek:
OK
PATH NOT FOUND (neexistující cesta)
 */
class CdCommand : public ICommand {

public:
    using ICommand::ICommand;
    using ICommand::process;
    using ICommand::registerFS;

private:
    std::vector<std::string> mAccumulator;

    bool validate_arguments() override;

    bool run() override;
};

/**
Vypíše aktuální cestu
pwd
Možný výsledek:
PATH
 */
class PwdCommand : public ICommand {

public:
    using ICommand::ICommand;
    using ICommand::process;
    using ICommand::registerFS;

private:
    bool validate_arguments() override;

    bool run() override;
};

/**
Vypíše informace o souboru/adresáři s1/a1 (v jakých clusterech se nachází)
info a1/s1
Možný výsledek:
S1 2,3,4,7,10
FILE NOT FOUND (není zdroj)
 */
class InfoCommand : public ICommand {

public:
    using ICommand::ICommand;
    using ICommand::process;
    using ICommand::registerFS;

private:
    bool validate_arguments() override;

    bool run() override;
};

/**
Nahraje soubor s1 z pevného disku do umístění s2 ve vašem FS
incp s1 s2
Možný výsledek:
OK
FILE NOT FOUND (není zdroj)
PATH NOT FOUND (neexistuje cílová cesta)
 */
class IncpCommand : public ICommand {

public:
    using ICommand::ICommand;
    using ICommand::process;
    using ICommand::registerFS;

private:
    DirectoryEntry mDestDE;
    std::vector<char> mBuffer;

    bool validate_arguments() override;

    bool run() override;
};

/**
Nahraje soubor s1 z vašeho FS do umístění s2 na pevném disku
outcp s1 s2
Možný výsledek:
OK
FILE NOT FOUND (není zdroj)
PATH NOT FOUND (neexistuje cílová cesta)
 */
class OutcpCommand : public ICommand {

public:
    using ICommand::ICommand;
    using ICommand::process;
    using ICommand::registerFS;

private:
    bool validate_arguments() override;

    bool run() override;
};

/**
Načte soubor z pevného disku, ve kterém budou jednotlivé příkazy, a začne je sekvenčně
vykonávat. Formát je 1příkaz/1řádek
load s1
Možný výsledek:
OK
FILE NOT FOUND (není zdroj)
 */
class LoadCommand : public ICommand {

public:
    using ICommand::ICommand;
    using ICommand::process;
    using ICommand::registerFS;

private:
    bool validate_arguments() override;

    bool run() override;
};

/**
Příkaz provede formát souboru, který byl zadán jako parametr při spuštění programu na
souborový systém dané velikosti. Pokud už soubor nějaká data obsahoval, budou přemazána.
Pokud soubor neexistoval, bude vytvořen.
format 600MB
Možný výsledek:
OK
CANNOT CREATE FILE
 */
class FormatCommand : public ICommand {

public:
    using ICommand::ICommand;
    using ICommand::process;
    using ICommand::registerFS;
private:

    bool validate_arguments() override;

    bool run() override;
};

/**
Defragmentace souboru – pokud login studenta začíná s-z
příkaz defrag s1 – Zajistí, že datové bloky souboru s1 budou ve filesystému uložené za sebou,
což si můžeme ověřit příkazem info. Předpokládáme, že v systému je dostatek místa, aby
nebyla potřeba přesouvat datové bloky jiných souborů.
 */
class DefragCommand : public ICommand {

public:
    using ICommand::ICommand;
    using ICommand::process;
    using ICommand::registerFS;

private:
    bool validate_arguments() override;

    bool run() override;
};


#endif //ZOS_SP_COMMANDS_H
