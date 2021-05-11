
int incomingByte = 0;   // lưu tín hiệu đến
void setup() {
    Serial.begin (115200); // (USB + TX/RX) to check
    Serial2.begin(115200); // GPIO 17: TXD U2  +  GPIO 16: RXD U2

    //UART_CONF0_REG  Configuration register 0
    //UART0: 0x3FF40020
    //UART1: 0x3FF50020
    //UART2: 0x3FF6E020

//    WRITE_PERI_REG( 0x3FF6E020 , READ_PERI_REG(0x3FF6E020) | (1<<16 ) | (1<<10 ) );  //UART_IRDA_EN + UART_IRDA_TX_EN  "Let there be light"
    //Serial.print("Reg: "); Serial.println(READ_PERI_REG(0x3FF6E020),BIN);  //For Debug only
}//setup

void loop() {
        if (Serial2.available() > 0) {
                // đọc các giá trị nhận được
                 

                // xuất ra những gì nhận được
//                Serial.print("Toi nghe duoc: ");
                Serial.print("Reg: " + String(Serial2.read()));
                Serial.println();
                Serial2.println("512");
        }
    
//    delay(1000);
} //loop
