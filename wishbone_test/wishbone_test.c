// SPDX-FileCopyrightText: 2023 Efabless Corporation

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//      http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <firmware_apis.h>
#define reg_wb_leds      (*(volatile uint32_t*)0x30000000)
#define reg_wb_buttons   (*(volatile uint32_t*)0x30000004)

void delay(const int d)
{

    /* Configure timer for a single-shot countdown */
	reg_timer0_config = 0;
	reg_timer0_data = d;
    reg_timer0_config = 1;

    // Loop, waiting for value to reach zero
   reg_timer0_update = 1;  // latch current value
   while (reg_timer0_value > 0) {
           reg_timer0_update = 1;
   }

}

void main(){
    // Enable managment gpio as output to use as indicator for finishing configuration  
    ManagmentGpio_outputEnable();
    ManagmentGpio_write(0);
    enableHkSpi(0); // disable housekeeping spi
    
    int pin = 0;
    /*
    .buttons (io_in [15:08]),
    .leds    (io_out[23:16]),
    .oeb     (io_oeb[23:16])
    */
    // buttons
    for(pin = 8 ;pin < 16; pin ++)
       GPIOs_configure(pin,GPIO_MODE_USER_STD_INPUT_PULLDOWN);

    // leds
    for(pin = 16; pin < 24; pin ++)
       GPIOs_configure(pin,GPIO_MODE_USER_STD_OUT_MONITORED);

    // send button status to management controlled gpio
    for(pin = 24; pin < 32; pin ++)
       GPIOs_configure(pin,GPIO_MODE_MGMT_STD_OUTPUT);
    
    GPIOs_loadConfigs(); // load the configuration 
    User_enableIF(); // turn on wishbone interface
    delay(1000);
    ManagmentGpio_write(1); // configuration finished 
    delay(1000);

    // https://caravel-sim-infrastructure.readthedocs.io/en/latest/C_api.html#_CPPv414USER_writeWordji
    while(1)
    {
        // read buttons. 0x1 is 4 byte offset, so reads from 0x3000_0004
        //uint32_t buttons = USER_readWord(0x1); //doesn't work
        uint32_t buttons = reg_wb_buttons; 

        // write the button status to leds. 0x0 is 4 byte offset, so writes to 0x3000_0000
        //USER_writeWord(buttons, 0x0); // doesn't work
        reg_wb_leds = buttons;

        // also output the buttons on the management controlled GPIOS
        GPIOs_writeLow(buttons << 24);

        // signal loop to the python testbench
        ManagmentGpio_write(0);
        ManagmentGpio_write(1);
    }

    return;
}