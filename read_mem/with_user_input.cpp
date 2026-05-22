#include <QDebug>
#include <QLoggingCategory>
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>
#include <system_error>
#include <QInputDialog>

/*
static QLoggingCategory category("\033[35m read_mem\033[0m");
static constexpr uintptr_t REG_BASE = 0xFF202780;
static constexpr size_t REG_SPAN = 0x80;*/

class FPGAConfig {


private:
    int fd;
    void* virtual_base;
    uint32_t page_size;
    uintptr_t page_addr;
    uintptr_t offset;

public:
    //static constexpr uintptr_t REG_BASE = 0xFF202780;
    static constexpr size_t REG_SPAN = 0x80;

    FPGAConfig() {
        /*
        fd = open("/dev/mem", O_RDWR | O_SYNC);
        if (fd < 0) {
            throw std::system_error(errno, std::generic_category(), "Open /dev/mem failed");
        }

        // Выравнивание адреса для mmap (кратность 4096 байт)
        page_size = sysconf(_SC_PAGESIZE);
        page_addr = REG_BASE & (~(static_cast<uintptr_t>(page_size) - 1));
        offset = REG_BASE - page_addr;

        virtual_base = mmap(nullptr, REG_SPAN + offset, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_addr);
        
        if (virtual_base == MAP_FAILED) {
            close(fd);
            throw std::system_error(errno, std::generic_category(), "mmap failed");
        }*/
    }

    ~FPGAConfig() {
        munmap(virtual_base, REG_SPAN + offset);
        close(fd);
    }

    // Метод для записи
    void write(uint32_t value) {
        *reinterpret_cast<volatile uint32_t*>(static_cast<uint8_t*>(virtual_base) + offset) = value;
    }

    // Метод для чтения
    uint32_t read(uintptr_t REG_BASE) const {
        int fd;
        //fd = open("/dev/mem", O_RDWR | O_SYNC);
        fd = open("/dev/str-mem", O_RDWR | O_SYNC);
        if (fd < 0) {
            throw std::system_error(errno, std::generic_category(), "Open /dev/mem failed");
        }

        // Выравнивание адреса для mmap (кратность 4096 байт)
        uint32_t page_size;
        uintptr_t page_addr;
        uintptr_t offset;
        void* virtual_base;
        page_size = sysconf(_SC_PAGESIZE);
        page_addr = REG_BASE & (~(static_cast<uintptr_t>(page_size) - 1));
        offset = REG_BASE - page_addr;

        virtual_base = mmap(nullptr, REG_SPAN + offset, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_addr);
        
        if (virtual_base == MAP_FAILED) {
            close(fd);
            throw std::system_error(errno, std::generic_category(), "mmap failed");
        }
        return *reinterpret_cast<volatile uint32_t*>(static_cast<uint8_t*>(virtual_base) + offset);
    }
};

int main() {
    try {

        static QLoggingCategory category("\033[35m read_mem\033[0m");
        
//static constexpr size_t REG_SPAN = 0x80;
        QTextStream qin(stdin);
        QString line = qin.readLine();
        bool ok;
        uintptr_t REG_BASE; // = 0xFF202780;
        REG_BASE = static_cast<uintptr_t>(line.toULongLong(&ok, 0)); // >> REG_BASE;

        if (ok) {
            qDebug(category) << "line" << line;
        // Success: 'address' contains the value
        } else {
        // Handling error: Conversion failed
        }
        
        // here stop
        FPGAConfig fpga;
        
        // Читаем значение
        uint32_t data = fpga.read(REG_BASE);
        //qDebug(category) << "1 Read value: 0x" << std::hex << data;
        qDebug(category) << "2 Read value: 0x" << data << QString::number(data,2);

        // Записываем значение
        // fpga.write(0x1);
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}