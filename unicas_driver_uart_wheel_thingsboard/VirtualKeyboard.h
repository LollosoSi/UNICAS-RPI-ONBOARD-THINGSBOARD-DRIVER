#include <fcntl.h>
#include <linux/uinput.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <stdexcept>

#include <fstream>

#pragma once

static void sendCommandToScript(const std::string& command) {
    const char* fifo_path = "/tmp/stream_control_pipe";
    std::ofstream pipe(fifo_path);
    if (!pipe) {
        std::cerr << "Errore: impossibile aprire la named pipe." << std::endl;
        return;
    }
    pipe << command << std::endl;
    pipe.close();
}

class VirtualKeyboard {
private:
    int fd;
    struct uinput_setup usetup;

    void emit(int type, int code, int val) {
        struct input_event ie;
        memset(&ie, 0, sizeof(ie));
        ie.type = type;
        ie.code = code;
        ie.value = val;
        write(fd, &ie, sizeof(ie));
    }

public:
    VirtualKeyboard() {
        fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (fd < 0) {
            throw std::runtime_error("Errore: impossibile aprire /dev/uinput (usa sudo o dai i permessi)");
        }

        // Abilita eventi di tastiera
        ioctl(fd, UI_SET_EVBIT, EV_KEY);
        ioctl(fd, UI_SET_EVBIT, EV_SYN);

        // Abilita tutti i tasti principali
        for (int code = KEY_ESC; code <= KEY_MICMUTE; ++code) {
            ioctl(fd, UI_SET_KEYBIT, code);
        }

        memset(&usetup, 0, sizeof(usetup));
        usetup.id.bustype = BUS_USB;
        usetup.id.vendor  = 0x1234; 
        usetup.id.product = 0x5678;
        strcpy(usetup.name, "Virtual Keyboard");

        ioctl(fd, UI_DEV_SETUP, &usetup);
        ioctl(fd, UI_DEV_CREATE);

        sleep(1); // tempo per far comparire il device
    }

    ~VirtualKeyboard() {
        if (fd >= 0) {
            ioctl(fd, UI_DEV_DESTROY);
            close(fd);
        }
    }

    void click(int keycode, int duration_ms = 50) {
        emit(EV_KEY, keycode, 1);  // pressione
        emit(EV_SYN, SYN_REPORT, 0);
        usleep(duration_ms * 1000);

        emit(EV_KEY, keycode, 0);  // rilascio
        emit(EV_SYN, SYN_REPORT, 0);
    }
};
