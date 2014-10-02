#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <avr/pgmspace.h>
#include “ip_arp_udp_tcp.h”
#include “enc28j60.h”
#include “timeout.h”
#include “net.h”
#include “websrv_help_functions.h”
 
static uint8_t mymac[6] = {0x54,0x55,0x58,0x10,0x00,0x30};
static uint8_t myip[4] = {192,168,0,25};
#define MYWWWPORT 80
#define LOGINPAGE 1
#define MAX_RELAY 6
 
static uint8_t relay2port[MAX_RELAY] = {0,1,2,3,4,5};
static char label[40]=”STEROWNIK DOMOWY – KRZYSZTOF LAPACZ”;
static char device[MAX_RELAY][20] = {“Went. łazienka”,”Went. kuchnia”,”Św. pokój”,”Św. korytarz”,”Św. półka_pokój”,”Św. kuchnia”};
static char password[9]=”coolciko”;
#define BUFFER_SIZE 660
static uint8_t buf[BUFFER_SIZE+1];
#define STR_BUFFER_SIZE 24
static char strbuf[STR_BUFFER_SIZE+1];

uint8_t verify_password(char *str)
{
  if (strncmp(password,str,strlen(password))==0)
    {
      return(1);
    }
 return(0);
}
 
int8_t analyse_get_url(char *str)
{
  uint8_t on=0;
  uint8_t port=0;
  
  if (LOGINPAGE==1)
    {
      if (find_key_val(str,strbuf,STR_BUFFER_SIZE,”pw”))
        {
          urldecode(strbuf);
          if (verify_password(strbuf))
            {
              return(3);
            }
        }
      if (*str == ‘ ‘)
        {
          return(4);
        }
      urldecode(str);
      
      if (verify_password(str))
        {
          while(on<16)
            {
              on++;
              str++;
              if (*str==’/’||*str==’?’)
                { 
                  on=0;
                  break;
                }
             }
           if (on)
            {
              return(-1);
           }
         }
       else
         {
           return(-1);
       }
     }
  else
    {
      if (*str != ‘ ‘ && *str != ‘?’)
        {
          return(0);
        }
    }
  if (find_key_val(str,strbuf,STR_BUFFER_SIZE,”sw”))
    {
      if (strbuf[0] != ‘p’)
        {
          return(0);
        }
      if (strbuf[1] != ‘c’)
        {
          return(0);
        }
      if (strbuf[2] < 0x3a && strbuf[2] > 0x2f)
        {
          port=strbuf[2]-0x30;
        }
      else
        {
          return(0);
        }
      if (find_key_val(str,strbuf,STR_BUFFER_SIZE,”a”))
        {
          if (strbuf[0] == ‘1’)
            {
              on=1;
            }
        }
    }
  if (port == 1)
    {
      if (on)
        {
          PORTC|= (1<<PORTC0);
        }
      else
        {
          PORTC &= ~(1<<PORTC0);
        }
    }
  if (port == 2)
    {
      if (on)
        {
          PORTC|= (1<<PORTC1);
        }
      else
        {
          PORTC &= ~(1<<PORTC1);
        }
    }
  if (port == 3)
    {
      if (on)
        {
          PORTC|= (1<<PORTC2);
        }
      else
        {
          PORTC &= ~(1<<PORTC2);
        }
    }
  if (port == 4)
  {
    if (on)
      {
        PORTC|= (1<<PORTC3);
      }
    else
      {
        PORTC &= ~(1<<PORTC3);
      }
  }
  if (port == 5)
    {
      if (on)
        {
          PORTC|= (1<<PORTC4);
        }
      else
        {
          PORTC &= ~(1<<PORTC4);
        }
    }
  if (port == 6)
    {
      if (on)
        {
          PORTC|= (1<<PORTC5);
        }
      else
        {
          PORTC &= ~(1<<PORTC5);
        }
    }
  return(1);
}

uint16_t moved_perm(uint8_t *buf)
  {
    uint16_t plen;
    plen=fill_tcp_data_p(buf,0,PSTR(“HTTP/1.0 301 Moved Permanently\r\nLocation: “));
    urlencode(password,strbuf);
    plen=fill_tcp_data(buf,plen,strbuf);
    plen=fill_tcp_data_p(buf,plen,PSTR(“/\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n”));
    plen=fill_tcp_data_p(buf,plen,PSTR(“<h1>301 Moved Permanently</h1>\n”));
    return(plen);
  }

uint16_t http200ok(void)
  {
    return(fill_tcp_data_p(buf,0,PSTR(“HTTP/1.0 200 OK\r\nContent-Type: text/html; \r\nPragma:
    no-cache\r\n\r\n”)));
  }
 
 
 
uint16_t print_webpage_login(uint8_t *buf)
  {
    uint16_t plen;
    plen=http200ok();
    plen=fill_tcp_data_p(buf,plen,PSTR(“<center><h2>”));
    plen=fill_tcp_data(buf,plen,label);
    plen=fill_tcp_data_p(buf,plen,PSTR(“</h2>\n<pre>”));
    plen=fill_tcp_data_p(buf,plen,PSTR(“<pre>”));
    plen=fill_tcp_data_p(buf,plen,PSTR(“<form action=/ method=get>”));
    plen=fill_tcp_data_p(buf,plen,PSTR(“HASŁO: <input type=password size=12 name=pw>\n”));
    plen=fill_tcp_data_p(buf,plen,PSTR(“<input type=submit value=\”login\”></form><hr></center>”));
    plen=fill_tcp_data_p(buf,plen,PSTR(“<small>By Ciko 2012″));
    return(plen);
  }

uint16_t print_webpage(uint8_t *buf)
  {
    uint16_t pl;
    uint8_t switchnum=0;
    pl=http200ok();
    pl=fill_tcp_data_p(buf,pl,PSTR(“<h2>”));
    pl=fill_tcp_data(buf,pl,label);
    pl=fill_tcp_data_p(buf,pl,PSTR(“</h2>\n<pre>”));
    pl=fill_tcp_data(buf,pl,”\n\n”);

    while(switchnum<MAX_RELAY)
      {
        if (PORTC & (1<<relay2port[switchnum]))
          {
            pl=fill_tcp_data_p(buf,pl,PSTR(“ON  – “));
          }
        else
          {
            pl=fill_tcp_data_p(buf,pl,PSTR(“OFF – “));
          }
        pl=fill_tcp_data_p(buf,pl,PSTR(” <a href=?sw=pc”));
        itoa(switchnum+1,strbuf,10);
        pl=fill_tcp_data(buf,pl,strbuf);
        pl=fill_tcp_data(buf,pl,”&a=”);
        if (PORTC & (1<<relay2port[switchnum]))
          {
            pl=fill_tcp_data(buf,pl,”0″);
          }
        else
          {
            pl=fill_tcp_data(buf,pl,”1″);
          }
        pl=fill_tcp_data_p(buf,pl,PSTR(“>”));
        pl=fill_tcp_data(buf,pl,device[switchnum]);
        pl=fill_tcp_data_p(buf,pl,PSTR(“</a>”));
        pl=fill_tcp_data(buf,pl,”\n”);
        switchnum++;
      }

    pl=fill_tcp_data_p(buf,pl,PSTR(“\n\n<a href=./\>[Odswież]</a>”));
    pl=fill_tcp_data_p(buf,pl,PSTR(“</pre><hr>”));
    pl=fill_tcp_data_p(buf,pl,PSTR(“<small>By Ciko 2012″));
    return(pl); 
  }
 
 
void enc28j60clkout(uint8_t clk)
{
//setup clkout: 2 is 12.5MHz:
  enc28j60Write(ECOCON, clk & 0x7);
}
 
int main(void)
{
  uint16_t plen;
  uint16_t dat_p;
  int8_t cmd;
  _delay_loop_1(0); // 60us

/*initialize enc28j60*/
  enc28j60Init(mymac);
  enc28j60clkout(2);
  _delay_loop_1(0); // 60us
  enc28j60PhyWrite(PHLCON,0x476);
  DDRC|= (1<<DDC0);
  DDRC|= (1<<DDC1);
  DDRC|= (1<<DDC2);
  DDRC|= (1<<DDC3);
  DDRC|= (1<<DDC4);
  DDRC|= (1<<DDC5);

  PORTC &= ~(1<<PORTC0);
  PORTC &= ~(1<<PORTC1);
  PORTC &= ~(1<<PORTC2);
  PORTC &= ~(1<<PORTC3);
  PORTC &= ~(1<<PORTC4);
  PORTC &= ~(1<<PORTC5);

  init_ip_arp_udp_tcp(mymac,myip,MYWWWPORT);
  while(1)
  {
    plen=enc28j60PacketReceive(BUFFER_SIZE, buf);
    dat_p=packetloop_icmp_tcp(buf,plen);
    if(dat_p==0)
    {
      continue;
    }
    if (strncmp(“GET “,(char *)&(buf[dat_p]),4)!=0)
    {
      plen=fill_tcp_data_p(buf,0,PSTR(“HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>200 OK</h1>”));
      goto SENDTCP;
    }
    cmd=analyse_get_url((char *)&(buf[dat_p+5]));
    if (cmd==-1)
    {
      plen=fill_tcp_data_p(buf,0,PSTR(“HTTP/1.0 401 Unauthorized\r\nContent-Type: text/html\r\n\r\n<h1>401 Unauthorized</h1>”));
      goto SENDTCP;
    }
    if (cmd==0)
    {
      plen=fill_tcp_data_p(buf,0,PSTR(“HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\n\r\n<h1>404 Not Found</h1>”));
      goto SENDTCP;
    }
    if (cmd==2)
    {
      plen=fill_tcp_data_p(buf,0,PSTR(“HTTP/1.0 406 Not Acceptable\r\nContent-Type: text/html\r\n\r\n<h1>406 IP addr. format wrong</h1>”));
      goto SENDTCP;
    }
    if (cmd==4)
    {
      plen=print_webpage_login(buf);
      goto SENDTCP;
    }
    if (cmd==3)
    {
      plen=moved_perm(buf);
      goto SENDTCP;
    }
    plen=print_webpage(buf);
 
SENDTCP:
    www_server_reply(buf,plen);
  }
 
return (0);
 
}
