#include <kos.h>
#include <dc/syscalls.h>
#include <sys/types.h>

class cd{
private:
public:
    cd();
    ~cd();
    bool checkStatus();
    void returnToBios();
    int status, disc_type;
};

cd::cd(){
    cdrom_get_status(&status, &disc_type);
}

cd::~cd(){
}

// Returns false if we're exiting to the bios
bool cd::checkStatus(){
    cdrom_get_status(&status, &disc_type);
    if (status == CD_STATUS_OPEN   || 
        status == CD_STATUS_NO_DISC || 
        status == CD_STATUS_RETRY  || 
        status == CD_STATUS_ERROR  || 
        status == CD_STATUS_FATAL) {
        return false;
    }
    return true;
}

void cd::returnToBios(){
    syscall_system_bios_menu();
}
