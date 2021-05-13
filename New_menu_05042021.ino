#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <string.h>

#define ON_OFF 23
#define TEST 13
#define ENTER 14
#define UP 18
#define DOWN 19
#define SET 5
#define Buzzer 4
#define PW 2

#define CMD_Write_Add_To_Light 200
#define CMD_Read_Add_From_Light 201
#define CMD_Test_Red 202
#define CMD_Test_Green 203
#define CMD_Test_Blue 204
#define CMD_Test_All 205
#define CMD_Trans_Limit_Power 206
#define CMD_Respond_To_Remote 208

typedef struct __attribute__((packed))
{
  uint8_t byte_start;
  uint8_t byte_CMD;
  uint8_t byte_data;
  uint8_t byte_end;
} RxBuff_Irda_t;

RxBuff_Irda_t RxBuff_Irda;

void send_struct(uint8_t *cb, uint8_t siz)
{
  uint8_t *buffer_send = cb;
  while (siz--)
  {
    Serial2.write(*buffer_send);
    buffer_send++;
  }
}
RxBuff_Irda_t RxBuff_Irda_recieve;
uint8_t inBuf[sizeof(RxBuff_Irda_t)];
uint8_t offset_index = 0;

void readstruct(uint8_t *pt, uint8_t size)
{
  uint16_t i = 0;
  uint8_t indRX = 0;
  for (i = 0; i < size; i++)
  {
    *pt = inBuf[indRX++];
    pt++;
  }
}

u8g2_uint_t offset;                             // current offset for the scrolling text
u8g2_uint_t width;                              // pixel width of the scrolling text (must be lesser than 128 unless U8G2_16BIT is defined
const char *text = "SIGNAL TECHNOLOGY CO.,LTD"; // scroll this text from right to left

//uint16_t check_sum(uint8_t *cb, uint8_t siz)
//{
//  uint8_t sum =0;
//  uint8_t *addr;
//  for(addr = (uint8_t*)cb; addr < (cb + siz); addr++){
//    // xor all the bytes
//    sum += *addr; // checksum = checksum xor value stored in addr
//  }
//  return sum;
//}

//U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
volatile bool is_touched_enter_key = false;
volatile bool is_touched_up_key = false;
volatile bool is_touched_down_key = false;

volatile bool is_back_home = false;
volatile bool is_back_set_address = false;
volatile bool is_back_respone_Address = false;

volatile bool is_finish_data_respone = false;

unsigned long time_begin = 0;

void Buzzer_key();

void drawText(void)
{
  u8g2_uint_t x = offset;
  do
  {
    u8g2.setFont(u8g2_font_6x13O_tr);   // repeated drawing of the scrolling text...
    u8g2.drawUTF8(x, 19, text);         // draw the scolling text
    x += width;                         // add the pixel width of the scrolling text
  } while (x < u8g2.getDisplayWidth()); // draw again until the complete display is filled
}

void setup(void)
{
  Serial.begin(115200);
  Serial2.begin(115200); // GPIO 17: TXD U2  +  GPIO 16: RXD U2
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_tr);
  pinMode(ON_OFF, INPUT);  // set ESP32 pin to input mode
  pinMode(Buzzer, OUTPUT); // set ESP32 pin to output mode
  pinMode(PW, OUTPUT);     // set ESP32 pin to output mode
  digitalWrite(PW, LOW);
}
struct menu_entry_type
{
  const uint8_t *font;
  uint16_t icon;
  const char *name;
};

struct menu_state
{
  int16_t menu_start;     /* in pixel */
  int16_t frame_position; /* in pixel */
  uint8_t position;       /* position, array index */
};

/*
  Icon configuration
  Width and height must match the icon font size
  GAP: Space between the icons
  BGAP: Gap between the display border and the cursor.
*/
#define ICON_WIDTH 32
#define ICON_HEIGHT 32
#define ICON_GAP 4
#define ICON_BGAP 16
#define ICON_Y 32 + ICON_GAP
//77 78 khoa phim // 69 test led
struct menu_entry_type menu_entry_list[] =
    {
        {u8g2_font_open_iconic_embedded_4x_t, 68, "Home"},
        {u8g2_font_open_iconic_embedded_4x_t, 80, "Set Address"},
        {u8g2_font_open_iconic_embedded_4x_t, 67, "Read Address"},
        {u8g2_font_open_iconic_embedded_4x_t, 69, "Led Test"},
        {u8g2_font_open_iconic_embedded_4x_t, 72, "Config Power"},
        {u8g2_font_open_iconic_embedded_4x_t, 78, "Lock Touch"},
        {u8g2_font_open_iconic_embedded_4x_t, 73, "Battery"},
        {u8g2_font_open_iconic_embedded_4x_t, 77, "About"},
        {NULL, 0, NULL}};

int8_t button_event = 0; // set this to 0, once the event has been processed

void check_button_event(void)
{
  if (button_event == 0)
  {
    if (digitalRead(ENTER))
    {
      while (digitalRead(ENTER))
      {
        Buzzer_key();
      }
      Serial.println("Enter touched\r\n");
      is_touched_enter_key = true;
      digitalWrite(Buzzer, LOW); // turn off Piezo Buzzer
    }
    if (digitalRead(UP))
    {
      while (digitalRead(UP))
      {
        Buzzer_key();
      }
      Serial.println("UP touched\r\n");
      is_touched_up_key = true;
      digitalWrite(Buzzer, LOW); // turn off Piezo Buzzer
    }
    if (digitalRead(DOWN))
    {
      while (digitalRead(DOWN))
      {
        Buzzer_key();
      }
      Serial.println("DOWN touched\r\n");
      is_touched_down_key = true;
      digitalWrite(Buzzer, LOW); // turn off Piezo Buzzer
    }
  }
}

void draw(struct menu_state *state)
{
  int16_t x;
  uint8_t i;
  x = state->menu_start;
  i = 0;
  while (menu_entry_list[i].icon > 0)
  {
    if (x >= -ICON_WIDTH && x < u8g2.getDisplayWidth())
    {
      u8g2.setFont(menu_entry_list[i].font);
      u8g2.drawGlyph(x, ICON_Y, menu_entry_list[i].icon);
    }
    i++;
    x += ICON_WIDTH + ICON_GAP;
    check_button_event();
  }
  u8g2.drawFrame(state->frame_position - 1, ICON_Y - ICON_HEIGHT - 1, ICON_WIDTH + 2, ICON_WIDTH + 2);
  u8g2.drawFrame(state->frame_position - 2, ICON_Y - ICON_HEIGHT - 2, ICON_WIDTH + 4, ICON_WIDTH + 4);
  u8g2.drawFrame(state->frame_position - 3, ICON_Y - ICON_HEIGHT - 3, ICON_WIDTH + 6, ICON_WIDTH + 6);
  check_button_event();
}

void to_right(struct menu_state *state)
{
  if (menu_entry_list[state->position + 1].font != NULL)
  {
    if ((int16_t)state->frame_position + 2 * (int16_t)ICON_WIDTH + (int16_t)ICON_BGAP < (int16_t)u8g2.getDisplayWidth())
    {
      state->position++;
      state->frame_position += ICON_WIDTH + (int16_t)ICON_GAP;
    }
    else
    {
      state->position++;
      state->frame_position = (int16_t)u8g2.getDisplayWidth() - (int16_t)ICON_WIDTH - (int16_t)ICON_BGAP;
      state->menu_start = state->frame_position - state->position * ((int16_t)ICON_WIDTH + (int16_t)ICON_GAP);
    }
  }
}

void to_left(struct menu_state *state)
{
  if (state->position > 0)
  {
    if ((int16_t)state->frame_position >= (int16_t)ICON_BGAP + (int16_t)ICON_WIDTH + (int16_t)ICON_GAP)
    {
      state->position--;
      state->frame_position -= ICON_WIDTH + (int16_t)ICON_GAP;
    }
    else
    {
      state->position--;
      state->frame_position = ICON_BGAP;
      state->menu_start = state->frame_position - state->position * ((int16_t)ICON_WIDTH + (int16_t)ICON_GAP);
    }
  }
}

uint8_t towards_int16(int16_t *current, int16_t dest)
{
  if (*current < dest)
  {
    (*current)++;
    return 1;
  }
  else if (*current > dest)
  {
    (*current)--;
    return 1;
  }
  return 0;
}

uint8_t towards(struct menu_state *current, struct menu_state *destination)
{
  uint8_t r = 0;
  uint8_t i;
  for (i = 0; i < 6; i++)
  {
    r |= towards_int16(&(current->frame_position), destination->frame_position);
    r |= towards_int16(&(current->menu_start), destination->menu_start);
  }
  return r;
}

struct menu_state current_state = {ICON_BGAP, ICON_BGAP, 0};
struct menu_state destination_state = {ICON_BGAP, ICON_BGAP, 0};

int Listening_respone_IR()
{
  memset(inBuf, 0, 4);
  while (Serial2.available())
  {
    uint8_t c = Serial2.read();
    //    Serial.println(c, HEX);
    inBuf[offset_index++] = c;
    if (offset_index == 4)
    {
      offset_index = 0;
      Serial.println("OK");
      return 1;
    }
  }
  return 0;
}

void loop(void)
{
  long int address = 1;
  long int scale_power = 0;
  int8_t event;
  /*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
  do
  {
    Buzzer_key();
    u8g2.firstPage();
    do
    {
      draw(&current_state);
      u8g2.setFont(u8g2_font_helvB10_tr);
      u8g2.setCursor((u8g2.getDisplayWidth() - u8g2.getStrWidth(menu_entry_list[destination_state.position].name)) / 2, u8g2.getDisplayHeight() - 5);
      u8g2.print(menu_entry_list[destination_state.position].name);
      check_button_event();
      delay(10);
    } while (u8g2.nextPage());
    if (is_touched_up_key)
    {
      is_touched_up_key = false;
      to_right(&destination_state);
      delay(10);
      continue;
    }
    if (is_touched_down_key)
    {
      is_touched_down_key = false;
      to_left(&destination_state);
      delay(10);
      continue;
    }
    if (is_touched_enter_key)
    {
      is_touched_enter_key = false;
      if (strcmp(menu_entry_list[destination_state.position].name, "Home") == 0)
      {
        Serial.println("Select: Home");
      }
      if (strcmp(menu_entry_list[destination_state.position].name, "Read Address") == 0)
      {
        Serial.println("Select: Read Address");
        is_back_home = false;
        int i = -1;
        while (!is_back_home)
        {
          u8g2.firstPage();
          do
          {
            Buzzer_key();
            /* assign a clip window and draw some text into it */
            u8g2.setFont(u8g2_font_ncenB08_tr);
            u8g2.setCursor(16, 13);
            u8g2.print("SITECH DMX 512");
            u8g2.drawFrame(0, 2, 128, 15);

            u8g2.setFont(u8g2_font_ncenB10_tr);
            u8g2.setCursor(0, 40);
            u8g2.print("Read Address: ");

            u8g2.setFont(u8g2_font_ncenB10_tr);
            u8g2.drawHLine(0, 50, 128);

            u8g2.setFont(u8g2_font_ncenB08_tr);
            u8g2.setCursor(2, 62);
            u8g2.print("Time Out -> BACK");

          } while (u8g2.nextPage());
          Buzzer_key();
          if (digitalRead(ON_OFF))
          {
            while (digitalRead(ON_OFF));
            is_back_home = true;
          }
          if (i < 0)
          {
            i = 0;
            RxBuff_Irda.byte_start = 0xFE;
            RxBuff_Irda.byte_CMD = CMD_Read_Add_From_Light;
            RxBuff_Irda.byte_data = 0;
            RxBuff_Irda.byte_end = 0XEF;
            send_struct((uint8_t *)&RxBuff_Irda, sizeof(RxBuff_Irda_t));
            Serial.println("Send CMD Read address");
          }

          Buzzer_key();
          if (Listening_respone_IR())
          {
            readstruct((uint8_t *)&RxBuff_Irda_recieve, sizeof(RxBuff_Irda_t));
            if (RxBuff_Irda_recieve.byte_start != 0xFE)
            {
              continue;
            }
            if (RxBuff_Irda_recieve.byte_end != 0xEF)
            {
              continue;
            }
            if (RxBuff_Irda_recieve.byte_CMD == CMD_Read_Add_From_Light)
            {
              Serial.print("Data ID recive: ");
              Serial.println(RxBuff_Irda_recieve.byte_data, DEC);
              is_back_respone_Address = false;
              delay(2000);
              while(!is_back_respone_Address)
              {
                u8g2.firstPage();
                do
                {
                  Buzzer_key();
                  /* assign a clip window and draw some text into it */
                  u8g2.setFont(u8g2_font_ncenB08_tr);
                  u8g2.setCursor(16, 13);
                  u8g2.print("SITECH DMX 512");
                  u8g2.drawFrame(0, 2, 128, 15);

                  u8g2.setFont(u8g2_font_ncenB10_tr);
                  u8g2.setCursor(0, 40);
                  u8g2.print("Address is: ");
                  u8g2.setFont(u8g2_font_logisoso16_tn);
                  u8g2.setCursor(90, 40);
                  u8g2.print(RxBuff_Irda_recieve.byte_data);

                  u8g2.setFont(u8g2_font_ncenB10_tr);
                  u8g2.drawHLine(0, 50, 128);

                  u8g2.setFont(u8g2_font_ncenB08_tr);
                  u8g2.setCursor(2, 62);
                  u8g2.print("Time Out -> BACK");

                } while (u8g2.nextPage());
                if (digitalRead(ON_OFF))
                {
                  Buzzer_key();
                  while (digitalRead(ON_OFF));
                  is_back_respone_Address = true;
                  is_back_home = true;
                }
              }
            }
          }
        }
      }
      if (strcmp(menu_entry_list[destination_state.position].name, "Battery") == 0)
      {
        Serial.println("Select: Battery");
      }
      if (strcmp(menu_entry_list[destination_state.position].name, "Config Power") == 0)
      {
        Serial.println("Select: Config Power");
        u8g2_uint_t x = 0;
        is_back_home = false;
        while (!is_back_home)
        {
          Buzzer_key();
          if (x == 0)
            x = 128;
          else
            x--;

          u8g2.firstPage();
          do
          {
            /* assign a clip window and draw some text into it */
            u8g2.setFont(u8g2_font_ncenB10_tr);
            u8g2.setClipWindow(0, 0, 128, 18); /* upper left and lower right position */
            u8g2.drawStr(x, 15, "SITECH DMX");

            /* remove clip window, draw to complete screen */
            u8g2.setMaxClipWindow();
            u8g2.drawFrame(0, 2, 128, 15);

            u8g2.setFont(u8g2_font_ncenB08_tr);
            u8g2.setCursor(0, 40);
            u8g2.print("Enter Scale:");
            u8g2.setFont(u8g2_font_ncenB12_tr);
            u8g2.setCursor(98, 40);
            u8g2.print("%");

            u8g2.setFont(u8g2_font_logisoso16_tn);
            u8g2.setCursor(65, 40);
            u8g2.print(scale_power);

            u8g2.setFont(u8g2_font_ncenB10_tr);
            u8g2.drawHLine(0, 50, 128);

            u8g2.setFont(u8g2_font_ncenB08_tr);
            u8g2.setCursor(2, 62);
            u8g2.print("Up/Down -> SET ");
          } while (u8g2.nextPage());

          Buzzer_key();
          if (digitalRead(ON_OFF))
          {
            while (digitalRead(ON_OFF));
            is_back_home = true;
          }
          if (digitalRead(UP))
          {
            Buzzer_key();
            while (digitalRead(UP));
            scale_power++;
            if (scale_power > 100)
            {
              scale_power = 0;
            }
          }
          if (digitalRead(DOWN))
          {
            Buzzer_key();
            while (digitalRead(DOWN));
            scale_power--;
            if (scale_power < 0)
            {
              scale_power = 100;
            }
          }
          if (digitalRead(SET))
          {
            while (digitalRead(SET));

            RxBuff_Irda.byte_start = 0xFE;
            RxBuff_Irda.byte_CMD = CMD_Trans_Limit_Power;
            RxBuff_Irda.byte_data = scale_power;
            RxBuff_Irda.byte_end = 0XEF;

            send_struct((uint8_t *)&RxBuff_Irda, sizeof(RxBuff_Irda_t));
            Serial.println("Struct write config Power------------");

            Serial.print("Byte start: ");
            Serial.print(RxBuff_Irda.byte_start, HEX);
            Serial.println();

            Serial.print("Byte CMD: ");
            Serial.print(RxBuff_Irda.byte_CMD, DEC);
            Serial.println();

            Serial.print("Data: ");
            Serial.print(RxBuff_Irda.byte_data, DEC);
            Serial.println();

            Serial.print("Byte end: ");
            Serial.print(RxBuff_Irda.byte_end, HEX);
            Serial.println();

            u8g2_uint_t x = 0;
            is_back_set_address = false;
            while (!is_back_set_address)
            {
              Buzzer_key();
              if (x == 0)
                x = 128;
              else
                x--;

              u8g2.firstPage();
              do
              {
                u8g2.setFont(u8g2_font_ncenB10_tr);
                u8g2.setClipWindow(0, 0, 128, 18); /* upper left and lower right position */
                u8g2.drawStr(x, 15, "SITECH DMX");

                /* remove clip window, draw to complete screen */
                u8g2.setMaxClipWindow();
                u8g2.drawFrame(0, 2, 128, 15);

                u8g2.setFont(u8g2_font_ncenB10_tr);
                u8g2.setCursor(0, 30);
                u8g2.print("Wait respond");

                // u8g2.setFont(u8g2_font_ncenB10_tr);
                // u8g2.setCursor(0, 50);
                // u8g2.print("Address: ");

                u8g2.setFont(u8g2_font_ncenB10_tr);
                u8g2.drawHLine(0, 52, 128);

                u8g2.setFont(u8g2_font_ncenB08_tr);
                u8g2.setCursor(2, 64);
                u8g2.print("Time out -> BACK ");
              } while (u8g2.nextPage());

              Buzzer_key();
              if (digitalRead(ON_OFF))
              {
                while (digitalRead(ON_OFF))
                  ;
                is_back_set_address = true;
              }
            }
          }
        }
      }
      if (strcmp(menu_entry_list[destination_state.position].name, "Lock Touch") == 0)
      {
        Serial.println("Select: Lock Touch");
      }
      if (strcmp(menu_entry_list[destination_state.position].name, "Led Test") == 0)
      {
        Serial.println("Select: Led Test");
        is_back_home = false;

        const char *string_list =
            "ON LED RED\n"
            "ON LED GREEN\n"
            "ON LED BLUE\n"
            "ON ALL";
        uint8_t current_selection = 1;

        while (!is_back_home)
        {
          Buzzer_key();
          current_selection = u8g2.userInterfaceSelectionList(
              "LED TEST",
              current_selection,
              string_list);

          if (current_selection == 0)
          {
            Buzzer_key();
            is_back_home = true;
          }
          else
          {
            char data[200] = "";
            strcpy(data, u8x8_GetStringLineStart(current_selection - 1, string_list));
            // Serial.println(data);

            // Serial.println(" Data filter");
            char *temp;
            temp = strtok((char *)&data, "\n");
            // Serial.println(temp);
            if (strcmp(temp, "ON LED RED") == 0)
            {
              Serial.println(" Turn on Red");
              RxBuff_Irda.byte_start = 0xFE;
              RxBuff_Irda.byte_CMD = CMD_Test_Red;
              RxBuff_Irda.byte_data = 0;
              RxBuff_Irda.byte_end = 0XEF;
              send_struct((uint8_t *)&RxBuff_Irda, sizeof(RxBuff_Irda_t));
            }
            if (strcmp(temp, "ON LED GREEN") == 0)
            {
              Serial.println("Turn on Green");
              RxBuff_Irda.byte_start = 0xFE;
              RxBuff_Irda.byte_CMD = CMD_Test_Green;
              RxBuff_Irda.byte_data = 0;
              RxBuff_Irda.byte_end = 0XEF;
              send_struct((uint8_t *)&RxBuff_Irda, sizeof(RxBuff_Irda_t));
            }
            if (strcmp(temp, "ON LED BLUE") == 0)
            {
              Serial.println("Turn on Blue");
              RxBuff_Irda.byte_start = 0xFE;
              RxBuff_Irda.byte_CMD = CMD_Test_Blue;
              RxBuff_Irda.byte_data = 0;
              RxBuff_Irda.byte_end = 0XEF;
              send_struct((uint8_t *)&RxBuff_Irda, sizeof(RxBuff_Irda_t));
            }
            if (strcmp(temp, "ON ALL") == 0)
            {
              Serial.println("Turn on all");
              RxBuff_Irda.byte_start = 0xFE;
              RxBuff_Irda.byte_CMD = CMD_Test_All;
              RxBuff_Irda.byte_data = 0;
              RxBuff_Irda.byte_end = 0XEF;
              send_struct((uint8_t *)&RxBuff_Irda, sizeof(RxBuff_Irda_t));
            }
          }
          if (digitalRead(ON_OFF))
          {
            while (digitalRead(ON_OFF))
              ;
            is_back_home = true;
          }
        }
      }
      if (strcmp(menu_entry_list[destination_state.position].name, "About") == 0)
      {
        Serial.println("Select: About");
        is_back_home = false;

        u8g2.setFont(u8g2_font_helvB18_tr); // set the target font to calculate the pixel width
        width = u8g2.getUTF8Width(text);    // calculate the pixel width of the text
        u8g2.setFontMode(0);                // enable transparent mode, which is faster
        u8g2.firstPage();
        do
        {
          u8g2.drawUTF8(18, 30, "SITECH"); // This part will stay constantly on the screen
        } while (u8g2.nextPage());

        while (!is_back_home)
        {
          int i;
          Buzzer_key();

          for (i = 0; i < 3; i++)
          {
            // draw to lines 0...23 (3*8-1)
            // draw to the upper part of the screen
            u8g2.setBufferCurrTileRow(i);
            u8g2.clearBuffer();
            drawText();

            // but write the buffer to the lower part (offset 4*8 = 32)
            u8g2.setBufferCurrTileRow(4 + i);
            u8g2.sendBuffer();
          }

          // calculate the new offset for the scrolling
          offset -= 1; // scroll by one pixel
          if ((u8g2_uint_t)offset < (u8g2_uint_t)-width)
            offset = 0; // start over again

          delay(10); // do some small delay

          if (digitalRead(ON_OFF))
          {
            Buzzer_key();
            while (digitalRead(ON_OFF))
              ;
            is_back_home = true;
          }
        }
      }
      if (strcmp(menu_entry_list[destination_state.position].name, "Set Address") == 0)
      {
        Serial.println("Select: Set Address");
        u8g2_uint_t x = 0;
        is_back_home = false;
        while (!is_back_home)
        {
          Buzzer_key();
          if (x == 0)
            x = 128;
          else
            x--;

          u8g2.firstPage();
          do
          {
            /* assign a clip window and draw some text into it */
            u8g2.setFont(u8g2_font_ncenB10_tr);
            u8g2.setClipWindow(0, 0, 128, 18); /* upper left and lower right position */
            u8g2.drawStr(x, 15, "SITECH DMX");

            /* remove clip window, draw to complete screen */
            u8g2.setMaxClipWindow();
            u8g2.drawFrame(0, 2, 128, 15);

            u8g2.setFont(u8g2_font_ncenB10_tr);
            u8g2.setCursor(0, 40);
            u8g2.print("Set Address: ");

            u8g2.setFont(u8g2_font_logisoso16_tn);
            u8g2.setCursor(98, 40);
            u8g2.print(address);

            u8g2.setFont(u8g2_font_ncenB10_tr);
            u8g2.drawHLine(0, 50, 128);

            u8g2.setFont(u8g2_font_ncenB08_tr);
            u8g2.setCursor(2, 62);
            u8g2.print("Up/Down -> SET ");
          } while (u8g2.nextPage());

          Buzzer_key();
          if (digitalRead(ON_OFF))
          {
            Buzzer_key();
            while (digitalRead(ON_OFF));
            is_back_home = true;
          }
          if (digitalRead(UP))
          {
            Buzzer_key();
            while (digitalRead(UP));
            address++;
            if (address > 170)
            {
              address = 1;
            }
          }
          if (digitalRead(DOWN))
          {
            Buzzer_key();
            while (digitalRead(DOWN));
            address--;
            if (address < 1)
            {
              address = 170;
            }
          }
          if (digitalRead(SET))
          {
            Buzzer_key();
            while (digitalRead(SET));

            RxBuff_Irda.byte_start = 0xFE;
            RxBuff_Irda.byte_CMD = CMD_Write_Add_To_Light;
            RxBuff_Irda.byte_data = address;
            RxBuff_Irda.byte_end = 0XEF;

            send_struct((uint8_t *)&RxBuff_Irda, sizeof(RxBuff_Irda_t));
        //     // Serial.println("Struct write address------------");

        //     // Serial.print("Byte start: ");
        //     // Serial.print(RxBuff_Irda.byte_start, HEX);
        //     // Serial.println();

        //     // Serial.print("Byte CMD: ");
        //     // Serial.print(RxBuff_Irda.byte_CMD, DEC);
        //     // Serial.println();

        //     // Serial.print("Data: ");
        //     // Serial.print(RxBuff_Irda.byte_data, DEC);
        //     // Serial.println();

        //     // Serial.print("Byte end: ");
        //     // Serial.print(RxBuff_Irda.byte_end, HEX);
        //     // Serial.println();

            u8g2_uint_t x = 0;
            is_back_set_address = false;
            while (!is_back_set_address)
            {
              Buzzer_key();
              if (x == 0)
                x = 128;
              else
                x--;

              u8g2.firstPage();
              do
              {
                u8g2.setFont(u8g2_font_ncenB10_tr);
                u8g2.setClipWindow(0, 0, 128, 18); /* upper left and lower right position */
                u8g2.drawStr(x, 15, "SITECH DMX");

                /* remove clip window, draw to complete screen */
                u8g2.setMaxClipWindow();
                u8g2.drawFrame(0, 2, 128, 15);

                u8g2.setFont(u8g2_font_ncenB10_tr);
                u8g2.setCursor(0, 30);
                u8g2.print("Wait respond");

                u8g2.setFont(u8g2_font_ncenB10_tr);
                u8g2.setCursor(0, 50);
                u8g2.print("Write Address");

                u8g2.setFont(u8g2_font_ncenB10_tr);
                u8g2.drawHLine(0, 52, 128);

                u8g2.setFont(u8g2_font_ncenB08_tr);
                u8g2.setCursor(2, 64);
                u8g2.print("Time out -> BACK ");
              } while (u8g2.nextPage());

              Buzzer_key();
              if (digitalRead(ON_OFF))
              {
                Buzzer_key();
                while (digitalRead(ON_OFF));
                is_back_set_address = true;
              }
              if (Listening_respone_IR())
              {
                readstruct((uint8_t *)&RxBuff_Irda_recieve, sizeof(RxBuff_Irda_t));
        //         // Serial.print("Byte start: ");
        //         // Serial.print(RxBuff_Irda_recieve.byte_start, HEX);
        //         // Serial.println();

        //         // Serial.print("Byte CMD: ");
        //         // Serial.print(RxBuff_Irda_recieve.byte_CMD, DEC);
        //         // Serial.println();

        //         // Serial.print("Data: ");
        //         // Serial.print(RxBuff_Irda_recieve.byte_data, DEC);
        //         // Serial.println();

        //         // Serial.print("Byte end: ");
        //         // Serial.print(RxBuff_Irda_recieve.byte_end, HEX);
        //         // Serial.println();

                if (RxBuff_Irda_recieve.byte_start != 0xFE)
                {
                  continue;
                }
                if (RxBuff_Irda_recieve.byte_end != 0xEF)
                {
                  continue;
                }
                if (RxBuff_Irda_recieve.byte_CMD == CMD_Respond_To_Remote)
                {
                  delay(2000);
                  if (RxBuff_Irda_recieve.byte_data)
                  {
                    u8g2_uint_t x = 0;
                    is_back_respone_Address = false;
                    while (!is_back_respone_Address)
                    {
                      Buzzer_key();
                      if (x == 0)
                        x = 128;
                      else
                        x--;
                      u8g2.firstPage();
                      do
                      {
                        u8g2.setFont(u8g2_font_ncenB10_tr);
                        u8g2.setClipWindow(0, 0, 128, 18); /* upper left and lower right position */
                        u8g2.drawStr(x, 15, "SITECH DMX");

                        /* remove clip window, draw to complete screen */
                        u8g2.setMaxClipWindow();
                        u8g2.drawFrame(0, 2, 128, 15);

                        u8g2.setFont(u8g2_font_ncenB10_tr);
                        u8g2.setCursor(0, 30);
                        u8g2.print("Write Address");

                        u8g2.setFont(u8g2_font_ncenB10_tr);
                        u8g2.setCursor(0, 50);
                        u8g2.print("Success -> BACK");

                        // u8g2.setFont(u8g2_font_ncenB10_tr);
                        // u8g2.drawHLine(0, 52, 128);

                        // u8g2.setFont(u8g2_font_ncenB08_tr);
                        // u8g2.setCursor(2, 64);
                        // u8g2.print("Time out -> BACK ");
                      } while (u8g2.nextPage());

                      if (digitalRead(ON_OFF))
                      {
                        Buzzer_key();
                        while (digitalRead(ON_OFF));
                        is_back_respone_Address = true;
                        is_back_set_address = true;
                      }
                    }
                  }
                  else
                  {
                    continue;
                  }
                } // IF check data read ir ok
              } // If listen IR
            } // While set respone
          } // If set
        }
      }
    }
    continue;
    is_touched_down_key = false;
    is_touched_enter_key = false;
    is_touched_up_key = false;
  } while (towards(&current_state, &destination_state));
}

  /*------------------------------------------------------------------------------------------------HAM CON-------------------------------------------------------------*/

  void Buzzer_key()
  {
    int touchState = digitalRead(ON_OFF) | digitalRead(TEST) | digitalRead(ENTER) | digitalRead(UP) | digitalRead(DOWN) | digitalRead(SET); // read new state
    if (touchState == HIGH)
    {
      digitalWrite(Buzzer, HIGH); // turn on Piezo Buzzer
    }
    else if (touchState == LOW)
    {
      digitalWrite(Buzzer, LOW); // turn off Piezo Buzzer
    }
  }

  /*------------------------------------------------------------------------------------------------HAM CON-------------------------------------------------------------*/
