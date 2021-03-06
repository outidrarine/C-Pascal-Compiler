#include <iostream>
#include <string>
#include "Hashage.h"
#include <math.h>
#include "Utilities.h"
using namespace std;

Hashage::Hashage():formattedTable('-', '|', '+')
{
	formattedTable.add("Index");
	formattedTable.add("Content");
	formattedTable.endOfRow();
	for (int i = 0; i < TABLESIZE; i++)
	{
		HashTable[i] = new node;
		HashTable[i]->data = "";
		HashTable[i]->next = NULL;
	}
}

void Hashage::ajouter(string data)
{

	int index = Hash(data);

	if (HashTable[index]->data == "")
	{
		HashTable[index]->data = data;
	}
	else
	{
		collisions++;
		node* n = new node;
		n->data = data;
		n->next = HashTable[index];
		HashTable[index] = n;
	}
}
int Hashage::nombreDesNoeuds(int index)
{
	int count = 0;

	if (HashTable[index]->data == "")
	{
		return count;
	}
	else
	{
		count++;
		node* Ptr = HashTable[index];
		while (Ptr->next != NULL)
		{
			count++;
			Ptr = Ptr->next;
		}
	}
	return count;
}
TextTable Hashage::afficher()
{
	int number;
	for (int i = 0; i < TABLESIZE; i++)
	{
		number = nombreDesNoeuds(i);

		if (number >= 1)
		{
			afficherNoeuds(i);
		}
	}
	return formattedTable;
}
void Hashage::afficherNoeuds(int index)
{
	node* Ptr = HashTable[index];

	if (Ptr->data == "")
	{
		// silence is golden --Echblaoui
	}
	else
	{
		while (Ptr != NULL)
		{
			formattedTable.add(to_string(index));
			formattedTable.add(Ptr->data);
			formattedTable.endOfRow();
			Ptr = Ptr->next;
		}

	}
}
bool Hashage::supprimer(string name)
{
	int index = Hash(name);

	node* delPtr;
	node* P1;
	node* P2;

	if (existe(name) == -1)// pr�condition
		return false; //non trouv�

	//Case 0 - seau vide
	if (HashTable[index]->data == "")
	{
		return false; // non trouv�
	}
	//Case 2 - un seul item dans le seau et c'est le item correspendant
	else if (HashTable[index]->data == name && HashTable[index]->next == NULL)
	{
		HashTable[index]->data = "";
		return true; // tr�s bien
	}
	//Case 3 - l'info existe dans le premier item du seau et il existe d'autres items dans le seau
	else if (HashTable[index]->data == name)
	{
		delPtr = HashTable[index];
		HashTable[index] = HashTable[index]->next;
		delete delPtr;
		collisions--;
		return true; // tr�s bien
	}
	//Case 3 - le seau contient des items et le premier item n'est pas le correspendant
	else
	{
		P1 = HashTable[index]->next;
		P2 = HashTable[index];
		while (P1 != NULL && P2->data != name)
		{
			P2 = P1;
			P1 = P1->next;
		}
		if (P1 == NULL)
		{
			return false; // non trouv�
		}
		else
		{
			delPtr = P1;
			P1 = P1->next;
			delete delPtr;
			collisions--;
			return true; // tr�s bien
		}

	}
}

int Hashage::Hash(string key) // hashage par la m�thode polynomiale et compression par methode de la division
{
	long hash1 = 0;
	int index;
	int coef = 42;
	for (int i = 0; i < key.length(); i++)
	{
		hash1 = hash1 + ((int)key[i]);
		hash1 = hash1 % TABLESIZE;
	}
	index = hash1 % TABLESIZE;
	return index;
}
int Hashage::existe(string info)
{
	int index = Hash(info);

	node* delPtr;
	node* P1;
	node* P2;

	//Case 0 - seau vide
	if (HashTable[index]->data == "")
	{
		return -1;
	}
	//Case 1 - un seul intem dans le deau, et c'est le intem correspendant
	else if (HashTable[index]->data == info)
	{
		return index;
	}
	//Case 2 - le seau contient des items et le premier n'est pas le correspendant
	else
	{
		P1 = HashTable[index]->next;
		P2 = HashTable[index];
		while (P1 != NULL && P2->data != info)
		{
			P2 = P1;
			P1 = P1->next;
		}
		if (P1 == NULL)
		{
			return -1;
		}
		else
		{
			return index;
		}

	}
}
string Hashage::get(int i)
{
	return HashTable[i]->data;
}