#include <stdlib.h>
#include <time.h>
#include "esp_sntp.h"
#include "time.h"

void iniciar_ntp(void)
{
    setenv("TZ", "WET0WEST,M3.5.0/1,M10.5.0", 1);
    tzset();

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
}



int64_t time_now(void){
    

    time_t agora = time(NULL);

    int64_t timestamp = (agora >= 1735689600) ? (int64_t)agora : 0;
 
    return timestamp; 

}
