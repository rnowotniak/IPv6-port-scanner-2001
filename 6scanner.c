/********************************************
 * 6scaner.c
 * Copyright (C) 2001   Robert M. Nowotniak <rnowotniak@gmail.com>
 * nie 9 gru 2001 14:37:58 CET
 *
 * Prosty skaner, do prostego skanowania adresów IPv6 (_tylko_ IPv6)
 * na wybranych portach. Kompilacja:
 * cc 6scaner.c -O<wedle uznania> -o 6scaner -Wall
 *
 * Oczekiwany format pliku wej¶ciowego (INFILE): adresy IPv6 w dowolnym,
 * prawid³owym dla IPv6 formacie - jeden pod drugim.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#define MAX_PORTOW 10
#define SECONDS 3 // Ile sekund do timeout'u
#define INFILE "INPUT"
#define OUTFILE "OUTPUT"


int wez_wpis(FILE*,char*);
int skanuj(char*,short*);
void timeout(int);
void flushuj(FILE*);

// Które porty skanowaæ ?
short porty[MAX_PORTOW]={21,80};

short fd;

int main(){
	FILE *wejscie,*wyjscie;
	char adres[40],tn=0;
	struct stat junk;
	short otwarte[MAX_PORTOW];
	int n;

	if(!(stat(OUTFILE,&junk))){
		while(tn!='t' && tn!='T' && tn!='n' && tn!='N'){
			printf("%s istnieje, usun±æ? [t/n]  ",OUTFILE);
			tn=getchar();
			flushuj(stdin);
			switch (tn){
				case 't':
				case 'T': if(unlink(OUTFILE)){
						printf("Nie mo¿na by³o usun±æ %s\n",OUTFILE);
						perror("B³±d");
						exit(1);
					  }
					  printf("Usuniêto %s\n",OUTFILE);
					  break;
				case 'n':
				case 'N':
					  exit(1);
					  break;
			}
		}
	}


	printf("Próba otwarcia pliku %s do odczytu i %s do zapisu...\n",INFILE,OUTFILE);
	if(!(wejscie=fopen(INFILE,"r")) || !(wyjscie=fopen(OUTFILE,"w"))  ){
		perror("B³±d");
		exit(1);
	}

	signal(SIGALRM,timeout);
	while(!wez_wpis(wejscie,adres)){
		bzero(otwarte,MAX_PORTOW);
		if(!(skanuj(adres,otwarte))){
			fprintf(wyjscie,"%s\t",adres);
			for(n=0;otwarte[n] && n<MAX_PORTOW;n++)
				fprintf(wyjscie," %d",otwarte[n]);
			fprintf(wyjscie,"\n");
		}
	}
	signal(SIGALRM,SIG_DFL);

	fclose(wejscie);
	fclose(wyjscie);

	exit(0);
}

int wez_wpis(FILE* strumien,char* dokad){
	char adres[40];

	if(fscanf(strumien,"%40s",adres)<=0)
		return -1;
	flushuj(strumien);

	strncpy(dokad,adres,40);

	return 0;

}

int skanuj(char* adres,short* zapis){
	struct sockaddr_in6* serv_addr;
	int wynik,n,m=0,ret=1;

	serv_addr=calloc(sizeof(struct sockaddr_in6),1);
	serv_addr->sin6_family=AF_INET6;


	printf("Skanowanie %s\n",adres);

	for(n=0;porty[n] && n<MAX_PORTOW;n++){
		if((fd=socket(AF_INET6,SOCK_STREAM,0))==-1){
			puts("Nie powiod³o siê utworzenie gniazda:");
			perror("B³±d");
			continue;
		}
		if((wynik=inet_pton(AF_INET6,adres,&serv_addr->sin6_addr))<0){
			puts("AF_INET6 nie wydaje siê byæ poprawn± rodzin± gniazd. (?)");
			close(fd);
			ret=1;
			break;
		}else if(!wynik){
			printf("Adres %s nie wydaje siê byæ poprawnym adresem IPv6.\n",adres);
			close(fd);
			ret=1;
			break;
		}
		serv_addr->sin6_port=htons(porty[n]);
		printf("Port %d... \t",porty[n]);

		alarm(SECONDS);
		if(connect(fd,(struct sockaddr*)serv_addr,sizeof(struct sockaddr_in6))){
			if(errno==EBADF){
				puts("timeout.");
				continue;
			}
			printf("failed.\n");
			printf("     %s\n",strerror(errno));
			close(fd);
			continue;
		}
		alarm(0);

		// Jest otwarty port

		puts("OK!");
		zapis[m++]=porty[n];
		ret=0;

		close(fd);

	}
	printf("\n");

	free((void*)serv_addr);

	return ret;
}

void timeout(int junk){
	close(fd);
}


void flushuj(FILE* co){
	char znak;
	do znak=getc(co);
		while(znak!='\n' && znak!=EOF);
}
