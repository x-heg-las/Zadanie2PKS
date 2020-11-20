#include "InitControll.h"
#include<iostream>

int chooseService()
{
	char choice;
	std::cout << "Vyber mod, v ktorom chcete pracovat (reciever\sender)\n[r\s] ";
	std::cin >> choice;
	
	if (choice == SENDER || choice == RECIEVER)
		return choice;

	return BAD_INPUT;
}

std::string loadIP()
{

	std::string input;
	std::cout << "Zadaj IP adresu prijmacieho zariadenia s pouzitim \".\" notacie . (x.x.x.x)" << std::endl;

	std::cin >> input;

	return input;
}


