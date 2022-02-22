#include <pic.h>

#define EMPTY_STRING "                "
#define HEX_NUMBER_SIZE 12
#define OCTAL_NUMBER_SIZE 16
#define BINARY_NUMBER_SIZE 48

unsigned char currentIndexLCD;

__CONFIG(0x3F72);



// ==========================================================LCD FUNCTIONS==========================================================

void delay(unsigned int x) {
    while (x--);
}

void pulse(unsigned int x) {
    RB2 = 1;
    delay(x);
    RB2 = 0;
    delay(x);
}

unsigned char isAnyButtonPressed() {
    RB4 = 1;
    RB5 = 1;
    RB6 = 1;
    RB7 = 1;
    if (RB4 && RB5 && RB6 && RB7) {
        delay(50000);
        RB4 = 1;
        RB5 = 1;
        RB6 = 1;
        RB7 = 1;
        if (RB4 && RB5 && RB6 && RB7) {
            return 0;
        }
        else {
            return 1;
        }
    }
    else {
        delay(50000);
        RB4 = 1;
        RB5 = 1;
        RB6 = 1;
        RB7 = 1;
        if (RB4 && RB5 && RB6 && RB7) {
            return 0;
        }
        else {
            return 1;
        }
    }
}

void sendCommandToLCD(unsigned char byte) {
    //while (isAnyButtonPressed());

    RB1 = 0;

    PORTB = (PORTB & 0x0F) + (byte & 0xF0);
    pulse(500);
    PORTB = (PORTB & 0x0F) + (byte << 4);
    pulse(500);

    RB1 = 1;

    delay(5000);
}

void setCoordLCD(unsigned char row, unsigned char col) {
    if (row == 0) {
        currentIndexLCD = col;
        sendCommandToLCD(0x80 + col);
    }
    else {
        currentIndexLCD = 16 + col;
        sendCommandToLCD(0xC0 + col);
    }
}

void sendByteToLCD(unsigned char byte) {
    //while (isAnyButtonPressed());

    PORTB = (PORTB & 0x0F) + (byte & 0xF0);
    pulse(500);
    PORTB = (PORTB & 0x0F) + (byte << 4);
    pulse(500);

    currentIndexLCD++;
    if (currentIndexLCD == 16)
        setCoordLCD(1, 0);
    if (currentIndexLCD == 32)
        setCoordLCD(0, 0);
}

void sendStringToLCD(const unsigned char* string) {
    while (*string) {
        sendByteToLCD(*(string++));
    }
}

void clearLCD() {
    setCoordLCD(0, 0);
    sendCommandToLCD(0x01); // Очистка экрана, счетчик адреса AC = 0, адресация AC на DDRAM (в данном случае на (0, 0))
}

void initLCD() {
    currentIndexLCD = 0;

    delay(5000);

    TRISB = 0;
    PORTB = 0x30;

    pulse(500);
    pulse(500);
    pulse(500);

    PORTB = 0x20;

    pulse(500);

    sendCommandToLCD(0x28); // 4 разряда данные, 2 строки, матрица 5х8 точек (0010 1000)
    sendCommandToLCD(0x0E); // Меняет режим отображения (0000 1110 - включено отображение и подчеркивание)
    sendCommandToLCD(0x06); // Меняет направление сдвига курсора или экрана (0000 0110 - увеличение счетчика адреса, без сдвига изображения)
    sendCommandToLCD(0x02); // AC = 0, адресация на DDRAM, сброшены сдвиги, начало строки адресуется в начале DDRAM (0000 0010)
}

void sendHexNumberToLCD(unsigned char* hexNumber, unsigned char hexNumberSize) {
    for (unsigned char i = 0; i < hexNumberSize; i++) {
        if (hexNumber[i] <= 9) {
            sendByteToLCD(hexNumber[i] + '0');
        }
        else {
            sendByteToLCD(hexNumber[i] - 10 + 'A');
        }
    }
}

void sendOctalNumberToLCD(unsigned char* octalNumber, unsigned char octalNumberSize) {
    for (unsigned char i = 0; i < octalNumberSize; i++) {
        sendByteToLCD(octalNumber[i] + '0');
    }
}



// ==========================================================EEPROM FUNCTIONS==========================================================

#define  testbit(var, bit)   ((var) & (1 <<(bit)))
#define  setbit(var, bit)    ((var) |= (1 << (bit)))
#define  clrbit(var, bit)    ((var) &= ~(1 << (bit)))

#define SCL 3
#define SDA 4

unsigned char checkACK, slaveAddress = 0xA0;

void LOW_SCL_I2C() {
    clrbit(PORTC, SCL);
    delay(5);
}

void HIGH_SCL_I2C() {
    setbit(PORTC, SCL);
    delay(5);
}

void LOW_SDA_I2C() {
    clrbit(PORTC, SDA);
    clrbit(TRISC, SDA);
    delay(5);
}

void HIGH_SDA_I2C() {
    //setbit(PORTC, SDA);
    setbit(TRISC, SDA);
    delay(5);
}

void CLOCK_PULSE_I2C() {
    HIGH_SCL_I2C();
    LOW_SCL_I2C();
}

void STOP_I2C() {
    LOW_SDA_I2C();
    LOW_SCL_I2C();
    HIGH_SCL_I2C();
    HIGH_SDA_I2C();
    LOW_SCL_I2C();
}

void START_I2C() {
    HIGH_SDA_I2C();
    HIGH_SCL_I2C();
    LOW_SDA_I2C();
    LOW_SCL_I2C();
}

void ACK_I2C() {
    LOW_SDA_I2C();
    CLOCK_PULSE_I2C();
}

void NACK_I2C() {
    HIGH_SDA_I2C();
    CLOCK_PULSE_I2C();
}

void CHECK_ACK_I2C() {
    HIGH_SCL_I2C();

    if (testbit(PORTC, SDA)) {
        checkACK = 1;
    }
    else {
        checkACK = 0;
    }

    LOW_SCL_I2C();
}

void OUT_BYTE_I2C(unsigned char byte) {
    unsigned char tmp = 8;

    while (tmp--) {
        if (byte & 0x80) {
            HIGH_SDA_I2C();
        }
        else {
            LOW_SDA_I2C();
        }
        CLOCK_PULSE_I2C();
        byte += byte;
    }

    HIGH_SDA_I2C();
    CHECK_ACK_I2C();
}

unsigned char IN_BYTE_I2C() {
    unsigned char byte = 0;
    unsigned char tmp = 8;

    HIGH_SDA_I2C();

    while (tmp--) {
        byte += byte;
        HIGH_SCL_I2C();
        if (testbit(PORTC, SDA)) {
            byte++;
        }
        LOW_SCL_I2C();
    }
    return byte;
}

unsigned char IN_BYTE_NACK_STOP_I2C() {
    unsigned char byte;
    byte = IN_BYTE_I2C();
    NACK_I2C();
    STOP_I2C();
    return byte;
}

unsigned char IN_BYTE_ACK_I2C() {
    unsigned char byte;
    byte = IN_BYTE_I2C();
    ACK_I2C();
    return byte;
}

void INIT_I2C() {
    RC4 = 0;
    TRISC3 = 0;
    TRISC4 = 0;
}

void INIT_WRITE_I2C(unsigned int address) {
rep:
    START_I2C();
    slaveAddress &= 0xFE;
    OUT_BYTE_I2C(slaveAddress);

    if (checkACK) {
        STOP_I2C();
        goto rep;
    }

    OUT_BYTE_I2C(address >> 8);

    if (checkACK) {
        STOP_I2C();
        goto rep;
    }

    OUT_BYTE_I2C(address);

    if (checkACK) {
        STOP_I2C();
        goto rep;
    }
}

void INIT_READ_I2C(unsigned int address) {
    INIT_WRITE_I2C(address);
    START_I2C();
    slaveAddress |= 1;
    OUT_BYTE_I2C(slaveAddress);
}



// ==========================================================MAIN FUNCTIONS==========================================================

void binaryToHex(unsigned char* binaryNumber, unsigned char binaryNumberSize, unsigned char* hexNumber, unsigned char hexNumberSize) {
    for (int i = 0; i < hexNumberSize; i++) {
        unsigned char hexDigit = 0;
        for (int j = 0; j < 4; j++) {
            hexDigit <<= 1;
            hexDigit |= 1 & binaryNumber[i * 4 + j];
        }
        hexNumber[i] = hexDigit;
    }
}

void binaryToOctal(unsigned char* binaryNumber, unsigned char binaryNumberSize, unsigned char* octalNumber, unsigned char octalNumberSize) {
    for (int i = 0; i < octalNumberSize; i++) {
        unsigned char octalDigit = 0;
        for (int j = 0; j < 3; j++) {
            octalDigit <<= 1;
            octalDigit |= 1 & binaryNumber[i * 3 + j];
        }
        octalNumber[i] = octalDigit;
    }
}

void hexToBinary(unsigned char* hexNumber, unsigned char hexNumberSize, unsigned char* binaryNumber, unsigned char binaryNumberSize) {
    for (int i = 0; i < hexNumberSize; i++) {
        for (int j = 0; j < 4; j++) {
            binaryNumber[i * 4 + j] = 1 & (hexNumber[i] >> (3 - j));
        }
    }
}

void saveResultToEEPROM(unsigned char* binaryNumber) {
rep:
    INIT_WRITE_I2C(0x0000);
    for (int i = 0; i < BINARY_NUMBER_SIZE / 8; i++) {  // Цикл по байтам числа binaryNumber (48 / 8 = 6 байт)
        unsigned char byte = 0;
        for (int j = 0; j < 8; j++) {   // Цикл по битам каждого байта
            byte <<= 1;
            byte |= 1 & binaryNumber[i * 8 + j];
        }
        OUT_BYTE_I2C(byte);
        if (checkACK) {
            STOP_I2C();
            goto rep;
        }
    }
    STOP_I2C();
}

void showLastResult() {
    unsigned char binaryNumber[BINARY_NUMBER_SIZE] = { 0 };

    INIT_READ_I2C(0x0000);
    for (int i = 0; i < BINARY_NUMBER_SIZE / 8; i++) {  // Цикл по байтам (48 / 8 = 6 байт)
        unsigned char byte = 0;
        if (i != (BINARY_NUMBER_SIZE / 8 - 1)) {   // Если это не последний байт
            byte = IN_BYTE_ACK_I2C();
        }
        else {  // Если это последний байт
            byte = IN_BYTE_NACK_STOP_I2C();
        }
        for (int j = 0; j < 8; j++) {   // Цикл по битам каждого байта
            binaryNumber[i * 8 + j] = 1 & (byte >> (7 - j));
        }
    }

    unsigned char hexNumber[HEX_NUMBER_SIZE] = { 0 };
    binaryToHex(binaryNumber, BINARY_NUMBER_SIZE, hexNumber, HEX_NUMBER_SIZE);

    unsigned char octalNumber[OCTAL_NUMBER_SIZE] = { 0 };
    binaryToOctal(binaryNumber, BINARY_NUMBER_SIZE, octalNumber, OCTAL_NUMBER_SIZE);

    while (isAnyButtonPressed());
    sendHexNumberToLCD(hexNumber, HEX_NUMBER_SIZE);
    setCoordLCD(1, 0);
    sendOctalNumberToLCD(octalNumber, OCTAL_NUMBER_SIZE);
}



// ==========================================================MAIN LOOP==========================================================

void mainLoop() {
    unsigned char hexNumber[HEX_NUMBER_SIZE] = { 0 };
    unsigned char currentIndexHexNumber = HEX_NUMBER_SIZE - 1;

    while (isAnyButtonPressed());
    sendHexNumberToLCD(hexNumber, HEX_NUMBER_SIZE);
    setCoordLCD(0, currentIndexHexNumber);

    while (1) {
        while (!isAnyButtonPressed());
        if (!RB4) { // Сдвиг курсора влево
            if (currentIndexHexNumber != 0) {
                currentIndexHexNumber--;
            }
            else {
                currentIndexHexNumber = HEX_NUMBER_SIZE - 1;
            }
            while (isAnyButtonPressed());
            setCoordLCD(0, currentIndexHexNumber);
        }
        else if (!RB5) {    // Сдвиг курсора вправо
            if (currentIndexHexNumber != (HEX_NUMBER_SIZE - 1)) {
                currentIndexHexNumber++;
            }
            else {
                currentIndexHexNumber = 0;
            }
            while (isAnyButtonPressed());
            setCoordLCD(0, currentIndexHexNumber);
        }
        else if (!RB6) {    // Увеличение числа под курсором
            if (hexNumber[currentIndexHexNumber] < 15) {
                hexNumber[currentIndexHexNumber]++;
            }
            else {
                hexNumber[currentIndexHexNumber] = 0;
            }
            //clearLCD();
            //sendHexNumberToLCD(hexNumber, HEX_NUMBER_SIZE);
            while (isAnyButtonPressed());
            if (hexNumber[currentIndexHexNumber] <= 9) {
                sendByteToLCD(hexNumber[currentIndexHexNumber] + '0');
            }
            else {
                sendByteToLCD(hexNumber[currentIndexHexNumber] - 10 + 'A');
            }
            setCoordLCD(0, currentIndexHexNumber);
        }
        else if (!RB7) {    // Перевод введенного числа в восьмиричную систему счисления
            unsigned char binaryNumber[BINARY_NUMBER_SIZE] = { 0 };
            hexToBinary(hexNumber, HEX_NUMBER_SIZE, binaryNumber, BINARY_NUMBER_SIZE);
            unsigned char octalNumber[OCTAL_NUMBER_SIZE] = { 0 };
            binaryToOctal(binaryNumber, BINARY_NUMBER_SIZE, octalNumber, OCTAL_NUMBER_SIZE);

            saveResultToEEPROM(binaryNumber);

            while (isAnyButtonPressed());
            setCoordLCD(1, 0);
            sendOctalNumberToLCD(octalNumber, OCTAL_NUMBER_SIZE);
            setCoordLCD(0, currentIndexHexNumber);

            while (!isAnyButtonPressed());
            while (isAnyButtonPressed());
            return;
        }
    }
}



// ==========================================================MAIN==========================================================

void main() {
    initLCD();
    INIT_I2C();
    while (isAnyButtonPressed());
    clearLCD();
    sendStringToLCD("Show last result?");
    setCoordLCD(1, 0);
    sendStringToLCD("RB4-Yes RB5-No");

    while (1) {
        while (!isAnyButtonPressed());
        if (!RB4) {
            while (isAnyButtonPressed());
            clearLCD();
            showLastResult();
            while (!isAnyButtonPressed());
            while (isAnyButtonPressed());
            break;
        }
        else if (!RB5) {
            while (isAnyButtonPressed());
            break;
        }
    }

    while (1) {
        initLCD();
        INIT_I2C();
        while (isAnyButtonPressed());
        clearLCD();
        mainLoop();
    }
}