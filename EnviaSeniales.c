#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>


int pid;
int opciones=0;
int main(){
	char * cad = malloc(30 * sizeof(char));
	char * cad1 = malloc(30 * sizeof(char));
	printf("Introduce el pid: ");
	scanf("%d",&pid);	
	while(opciones!=8){
		printf("Introduzca una opción: \n");
		printf("	1.- SIGUSR1\n");
		printf("	2.- SIGUSR2\n");
		printf("	3.- SIGUSR1+SIGUSR2\n");
		printf("	4.- SIGUSR1+SIGUSR1\n");
		printf("	5.- SIGUSR2+SIGUSR2\n");
		printf("	6.- SIGINT\n");
		printf("	7.- Modificar PID\n");
		printf("	8.- Salir\n");
		scanf("%d",&opciones);
		switch(opciones){
		case 1:
			kill(pid, SIGUSR1);
			break;
		case 2:	
			kill(pid, SIGUSR2);
			break;
		case 3:
			kill(pid, SIGUSR1);
			kill(pid, SIGUSR2);
			break;
		case 4:
			kill(pid, SIGUSR1);
			kill(pid, SIGUSR1);
			break;
		case 5:
			kill(pid, SIGUSR2);
			kill(pid, SIGUSR2);
			break;
		case 6:
			kill(pid, SIGINT);
			break;
		case 7:
			printf("Introduzca el nuevo PID: \n");
			scanf("%d", &pid);
			break;
		case 8:
			return 0;
		default:
			printf("OPCION NO VALIDA");
		}
	}
}
