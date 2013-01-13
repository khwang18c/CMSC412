#include <net.h>
#include <conio.h>
#include <string.h>
#include <geekos/errno.h>
#include <ip.h>

int main(int argc, char **argv) {
    bool ipValid;
    uchar_t ipAddress[4];
    uchar_t macAddress[6];
    int rc = 0;
    int i;

    if (argc != 2) {
        Print("Usage:\n\t%s ip_address\n", argv[0]);
        return -1;
    }

    ipValid = Parse_IP(argv[1], ipAddress);
    if (!ipValid) {
        Print("IP Address %s not valid\n", argv[1]);
        return -2;
    }

    rc = Arp(ipAddress, macAddress);
    if (rc == ETIMEOUT) {
        Print("ARP timed out\n");
        return rc;
    } else if (rc != 0) {
        Print("ARP failed with error code %d\n", rc);
        return rc;
    }

    Print("Found MAC address: ");
    for (i = 0; i < 6; ++i) {
        Print("%x:", macAddress[i]);
    }

    Print("\n");


    return 0;
}
