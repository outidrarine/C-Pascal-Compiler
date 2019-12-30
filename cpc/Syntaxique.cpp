#pragma warning( push )
#pragma warning( disable : 26444 ) //Disabling warnings generated by souhail's code
#include "Syntaxique.h"
#include "Lexical.h"
using namespace std;

//Constructeurs
Syntaxique::Syntaxique(string inputFile, bool logTableIden, bool logMotsRes)
{
	lexical = new Lexical(inputFile, true, logTableIden,logMotsRes);
	semantique = new Semantique();  
	generator = new ObjectCodeGenerator(lexical->getInputFileNameWithoutExt());
	uniteCourante = { END,0 };
	ite_varlocalglobal = 0;
	nbrDeclarations = 0;
	nbrParam = 0;

	if (lexical->getInputFileNameWithoutExt() == "")
		logError("File name not specified.");
	lexical->setInput(inputFile, logTableIden, logMotsRes);
}

Syntaxique::Syntaxique(Lexical* pLexical)
{
	lexical = pLexical;
	semantique = new Semantique();
	generator = new ObjectCodeGenerator(lexical->getInputFileNameWithoutExt());
	uniteCourante = { END,0 };
	ite_varlocalglobal = 0;
	nbrDeclarations = 0;
	nbrParam = 0;

	if (lexical->getInputFileNameWithoutExt() == "")
		logError("File name not specified.");
}

//Functions

/// <summary>
///	 Starts checking the syntax of the current file loaded in the lexical parser
/// </summary>
void Syntaxique::startParsing()
{
	//Initialisation du generateur de code
	//generator = new ObjectCodeGenerator(lexical->getInputFileNameWithoutExt());
	// Gestion du fichier XML
	xmlFile.open(XML_DIRECTORY + "/" + lexical->getInputFileNameWithoutExt() + ".xml");
	if (!xmlFile.is_open())
		logError("Couldn't open xml file, check if xml dir is already created.");

	uniteCourante = lexical->uniteSuivante();
	programme();
	semantique->ControlerTS(); //Souhail a revoir
	if (uniteCourante.UL != END) {
		syntaxError(eEnd);
	}
	else
		printErrors();	//affiche toutes les erreurs trouv�es
	end(); //finish him!
}

bool Syntaxique::isMotCle(string mc)
{
	if (uniteCourante.UL == MOTCLE) {
		if (uniteCourante.attribut == lexical->identifiants.existe(mc)) {
			return true;
		}
		else { return false; }
	}
	else { return false; }
}

void Syntaxique::end() {
	//Logging the errors : We're trying to be precise and avoid printing too much errors
	//First, we check if there's multiple errors targeting the same Ln&Col
	generator->end();
	for (auto const& element : syntaxErrors) {
		auto line = element.first.first;
		auto col = element.first.second;
		string message =  lexical->inputFilename + ":" + "at Ln " + to_string(line) + ", Col "+ to_string(col) +  ": expected ";
		for (auto const& symbol : element.second)
			message += symbol + " ";
		logError(message);
	}
	//Writing XML file
	xmlFile.flush();
}

void Syntaxique::printErrors() {
	 
}

void Syntaxique::xmlOpen(string caller) {
	xmlFile << "<" << caller << ">";
}

void Syntaxique::xmlClose(string caller)
{
	xmlFile << "</" << caller << ">";
}
//R�gles de grammaire
void Syntaxique::programme() {
	xmlOpen("programme");
	if (estPremierDe(eListeDeDeclarations)) {
		listeDeDeclarations();
		ite_varlocalglobal++;
		//reservation de mem pour les var globales
		generator->pile(nbrDeclarations);
		nbrDeclarations = 0;
		//appel du main
		generator->appel("main");
		generator->fin();
		listeDeFonctions();
	}
	else {
		syntaxError(eProgramme);
	}
	xmlClose("programme");
}

void Syntaxique::listeDeFonctions()
{
	xmlOpen("listeDeFonctions");
	if (estPremierDe(eListeDeFonctions) && uniteCourante.UL != MAIN)
	{
		fonction();
		listeDeFonctions();
	}
	else if (uniteCourante.UL == MAIN) // TODO: mettre une production pour le main seul ?
	{
		xmlOpen("main");
		//actions semantiques
		semantique->AjouterTS("val", lexical->identifiants.get(uniteCourante.attribut), ite_varlocalglobal);  
		(semantique->TS.end() - 1)->type = "fct"; 

		//gen de code-
		generator->entree("main");
		currentFunction = "main";
		adresses.insert(pair<int, string>{lexical->getLine() - 1, "main"});
		//syntax checking
		uniteCourante = lexical->uniteSuivante();

		consommer(PAROUV);
		listeDeParametres();

		//gen code
		/*generator->pile(nbrParam);
		nbrParam = 0;*/

		consommer(PARFERM);
		semantique->paramFonctTS();

		listeDeDeclarations();

		//gen code
		generator->pile(nbrDeclarations);
		nbrDeclarations = 0;

		consommer(ACCOUV);
		listeInstructions();
		consommer(ACCFERM);
		ite_varlocalglobal++;
		xmlClose("main");
	}
	else
	{
		syntaxError(eFonction);
	}
	xmlClose("listeDeFonctions");
}

void Syntaxique::fonction()
{
	xmlOpen("fonction");
	if (estPremierDe(eIdentificateur))
	{

		//actions semantiques
		semantique->AjouterTS("val", lexical->identifiants.get(uniteCourante.attribut), ite_varlocalglobal);  
		(semantique->TS.end() - 1)->type = "fct"; 

		//gen de code
		currentFunction = lexical->identifiants.get(uniteCourante.attribut);
		generator->entree(currentFunction);
		adresses.insert(pair<int, string>{lexical->getLine() - 1, lexical->identifiants.get(uniteCourante.attribut)});
		//generator->pile(3);//offset

		//syntax checking
		consommer(IDENT);
		consommer(PAROUV);
		listeDeParametres();

		//gen code
		/*generator->pile(nbrParam);
		nbrParam = 0;*/

		consommer(PARFERM);
		semantique->paramFonctTS();

		listeDeDeclarations();

		//gen code
		generator->pile(nbrDeclarations);
		nbrDeclarations = 0;

		consommer(ACCOUV);
		listeInstructions();
		consommer(ACCFERM);
		ite_varlocalglobal++;
	}
	else
	{
		syntaxError(eIdentificateur);
	}
	xmlClose("fonction");
}

void Syntaxique::listeDeDeclarations()
{
	xmlOpen("listeDeDeclarations");
	if (estPremierDe(eListeDeDeclarations))
	{
		declarations();
		consommer(PTVRG);
	}
	else if (estSuivantDe(eListeDeDeclarations))
	{
		//silence is golden
	}
	else
	{
		syntaxError(eListeDeDeclarations);
	}
	xmlClose("listeDeDeclarations");
}

void Syntaxique::declarations()
{
	xmlOpen("declarations");
	if (estPremierDe(eDeclarations))
	{
		declaration();
		declarationsPrime();
	}
	else {
		syntaxError(eDeclaration);
	}
	xmlClose("declarations");
}

void Syntaxique::declarationsPrime()
{
	xmlOpen("declarationsPrime");
	if (estPremierDe(eDeclarationsPrime))
	{
		consommer(VIRG);
		declaration();
		declarationsPrime();
	}
	else if (estSuivantDe(eDeclarationsPrime)) {
		//silence is golden
	}
	else {
		syntaxError(eDeclarationsPrime);
	}
	xmlClose("declarationsPrime");
}

void Syntaxique::declaration()
{
	xmlOpen("declaration");
	if (uniteCourante.UL == ENTIER)
	{
		nbrDeclarations++;
		semantique->AjouterTS("type", "entier", ite_varlocalglobal);  
		consommer(ENTIER);
		declarationPrime();
	}
	else if (uniteCourante.UL == CAR)
	{
		nbrDeclarations++;
		semantique->AjouterTS("type", "car", ite_varlocalglobal);
		consommer(CAR);
		declarationPrime();
	}
	else
	{
		syntaxError(eDeclaration);
	}
	xmlClose("declaration");
}

void Syntaxique::declarationPrime()
{
	xmlOpen("declarationPrime");
	if (estPremierDe(eIdentificateur))
	{
		(*semantique).TS.back().nom = lexical->identifiants.get(uniteCourante.attribut); 
		consommer(IDENT);
		declarationSeconde();
	}
	else {
		syntaxError(eDeclaration);
	}
	xmlClose("declarationPrime");
}

void Syntaxique::declarationSeconde()
{
	xmlOpen("declarationSeconde");
	if (estPremierDe(eDeclarationSeconde))
	{
		consommer(CROOUV);
		(*semantique).TS.back().nb = 1; 
		expression();
		consommer(CROFER);
	}
	else if (estSuivantDe(eDeclarationSeconde))
	{
		//silence is golden
	}
	else
	{
		syntaxError(eDeclarationSeconde);
	}
	xmlClose("declarationSeconde");
}

void Syntaxique::listeDeParametres()
{
	xmlOpen("listeDeParametres");
	if (estPremierDe(eParametre))
	{
		parametres();
	}
	else if (estSuivantDe(eListeParametres))
	{
		//silence is golden
	}
	else
	{
		syntaxError(eListeParametres);
	}
	xmlClose("listeDeParametres");
}

void Syntaxique::parametres()
{
	xmlOpen("parametres");
	if (estPremierDe(eParametre)) {
		parametre();
		parametresPrime();
	}
	else { syntaxError(eParametres); }
	xmlClose("parametres");
}

void Syntaxique::parametresPrime()
{
	xmlOpen("parametresPrime");
	if (estPremierDe(eParametresPrime)) {
		consommer(VIRG);
		parametre();
		parametresPrime();
	}
	else if (estSuivantDe(eParametresPrime))
	{
		//silence is golden
	}
	else { syntaxError(eParametresPrime); }
	xmlClose("parametresPrime");
}

void Syntaxique::parametre()
{
	xmlOpen("parametre");
	if (estPremierDe(eParametre)) {
		nbrParam++;
		if (uniteCourante.UL == ENTIER) {
			consommer(ENTIER);
			semantique->AjouterTS("type", "entier", ite_varlocalglobal);  
		}
		else if (uniteCourante.UL == CAR) {
			consommer(CAR);
			semantique->AjouterTS("type", "car", ite_varlocalglobal);  
		}

		if (uniteCourante.UL == IDENT) {
			(*semantique).TS.back().nom = lexical->identifiants.get(uniteCourante.attribut); 
		}
		identif();
	}
	else if (uniteCourante.UL == ENTIER)
	{
		consommer(ENTIER);
		identif();

	}
	else { syntaxError(eParametre); }
	xmlClose("parametre");
}

void Syntaxique::listeInstructions()
{
	xmlOpen("listeInstructions");
	if (estPremierDe(eInstruction)) {
		instruction();
		listeInstructions();
	}
	else if (estSuivantDe(eListeInstructions))
	{
		//silence is golden
	}
	else { 
		syntaxError(eListeInstructions);
	}
	xmlClose("listeInstructions");
}

void Syntaxique::instruction() //URFENT TODO: REMOVE IS MOT CLE
{
	//vars gen de code
	static int counter = 0; // incrementé à chaque branchement conditionel pour distinguer les labels

	string porteur;
	xmlOpen("instruction");
	string tempIdent;
	if (estPremierDe(eIdentificateur)) {
		//semantique 
		tmp = lexical->identifiants.get(uniteCourante.attribut); 
		porteur = semantique->typeidentifTS(tmp); 

		tempIdent = lexical->identifiants.get(uniteCourante.attribut);

		identif();
		instructionPrime(porteur);

		//gen de code
		variable data = semantique->getVariableData(tempIdent, ite_varlocalglobal);

		if (data.type == "entier" || data.type == "car") {
			if (data.scope == 0)//variable globale
				generator->depg(semantique->getVariableAddress(tempIdent, ite_varlocalglobal));
			else {
				generator->depl(semantique->getVariableAddress(tempIdent, ite_varlocalglobal));
			}
		}

		consommer(PTVRG);
	}
	else if (uniteCourante.UL == RETOUR)
	{
		consommer(RETOUR);
		expression();
		int nbrParam = semantique->getVariableData(currentFunction, ite_varlocalglobal).param.size();
		generator->depl(-1 - nbrParam - 1); // -1 pour adr retour
		generator->sortie();
		generator->retour();
		consommer(PTVRG);
	}
	else if (uniteCourante.UL == SI)
	{
		consommer(SI);
		expression();
		int tempCounter = counter; // solution pour les if imbriqués
		counter++;
		generator->sifaux("finsi " + to_string(tempCounter));
		if (uniteCourante.UL == ALORS) {
			consommer(ALORS);
			consommer(ACCOUV);
			listeInstructions();
			consommer(ACCFERM);
			generator->label("finsi " + to_string( tempCounter));
			adresses.insert(adresses.begin(), pair<int, string>(lexical->getLine()-1, "finsi " + to_string(tempCounter)));
			instructionSeconde();
		}
		else { return syntaxError(eInstruction); }
	}
	else if (uniteCourante.UL == TANTQUE)
	{
		consommer(TANTQUE);
		consommer(PAROUV);
		expression();
		consommer(PARFERM);
		int tempCounter = counter; // solution pour les if imbriqués
		counter++;
		if (uniteCourante.UL == FAIRE) {
			generator->sifaux("fintantque " + to_string(tempCounter));
			generator->label("debuttantque " + to_string(tempCounter));
			adresses.insert(adresses.begin(), pair<int, string>(lexical->getLine()-1, "debuttantque " + to_string(tempCounter)));
			consommer(FAIRE);
			consommer(ACCOUV);
			listeInstructions();
			consommer(ACCFERM);
			generator->saut("debuttantque " + to_string(tempCounter));
			generator->label("fintantque " + to_string(tempCounter));
			adresses.insert(adresses.begin(), pair<int, string>(lexical->getLine()-1, "fintantque " + to_string(tempCounter)));
		}
		else { return syntaxError(eInstruction); }
	}
	else if (uniteCourante.UL == ECRIRE) {
		consommer(ECRIRE);
		consommer(PAROUV);
		expression();
		generator->ecriv();
		consommer(PARFERM);
		consommer(PTVRG);
	}
	else { return syntaxError(eInstruction); }
	xmlClose("instruction");//retard dair open hna
}

void Syntaxique::instructionPrime()
{
	xmlOpen("instructionPrime");
	if (estPremierDe(eInstructionPrime)) {
		consommer(EGAL);
		instructionTriple();
	}
	else if (uniteCourante.UL == CROFER) {
		consommer(CROOUV);
		expression();
		consommer(CROFER);
		consommer(EGAL);
		instructionTriple();
	}
	else { syntaxError(eInstructionPrime); }
	xmlClose("instructionPrime");
}

void Syntaxique::instructionPrime(string instruprime) 
{
	xmlOpen("instructionPrime");
	if (estPremierDe(eInstructionPrime)) {
		consommer(EGAL);
		string instrutriple = instructionTriple(); 
		if (instrutriple != instruprime && instrutriple !="fct" ) 
			semantique->logError("conflicting type " + instrutriple + " differe de " + instruprime);
	}
	else if (uniteCourante.UL == CROFER) {
		consommer(CROOUV);
		if (semantique->VerifierTableau(tmp) == false) 
			semantique->logError("incompatible type: expected a table");
		expression();
		consommer(CROFER);
		consommer(EGAL);
		string instrutriple = instructionTriple(); 
		if (instrutriple != instruprime && instrutriple != "fct" ) 
			semantique->logError("conflicting type " + instrutriple + " differe de " + instruprime);
	}
	else { syntaxError(eInstructionPrime); }
	xmlClose("instructionPrime");
}

void Syntaxique::instructionSeconde()
{
	xmlOpen("instructionSeconde");
	if (uniteCourante.UL == SINON) {
		consommer(SINON);
		consommer(ACCOUV);
		listeInstructions();
		consommer(ACCFERM);
	}
	else if (estSuivantDe(eInstructionSeconde))
	{
		//silence is golden
	}
	//else { syntaxError(eInstructionSeconde); }
	xmlClose("instructionSeconde");
}

string Syntaxique::instructionTriple()
{
	int i = 0;
	xmlOpen("instructionTriple");
	if (uniteCourante.UL == LIRE) {
		consommer(LIRE);
		generator->lire();
		consommer(PAROUV);
		consommer(PARFERM); 
		i++;
	}
	else if (estPremierDe(eExpression)) {
		expression();
		i++;
	}
	else { syntaxError(eInstructionTriple); }
	xmlClose("instructionTriple");
	if (i != 0) return expr;
		else return string();
}

void Syntaxique::expression()
{
	xmlOpen("expression");
	if (estPremierDe(eExpressionLogique)) {
		expressionLogique();
		expressionPrime();
	}
	else { syntaxError(eExpression); }
	xmlClose("expression");
}

void Syntaxique::expressionPrime()
{
	xmlOpen("expressionPrime");
	if (estPremierDe(eOperateurLogique)) {
		operateurLogique();
		expressionLogique();
		expressionPrime();
	}
	else if (estSuivantDe(eExpressionPrime)) {
		//silence is golden
	}
	else { syntaxError(eExpressionPrime); }
	xmlClose("expressionPrime");
}

void Syntaxique::expressionLogique() {
	xmlOpen("expressionLogique");
	if (estPremierDe(eExpressionSimple)) {
		expressionSimple();
		expressionLogiquePrime();
	}
	else {
		syntaxError(eExpressionLogique);
	}
	xmlClose("expressionLogique");
}

void Syntaxique::expressionLogiquePrime() {
	xmlOpen("expressionLogiquePrime");
	if (estPremierDe(eComparaison)) {
		string comp = comparaison();
		expressionSimple();
		generator->comp(comp);
		expressionLogiquePrime();
		
	}
	else if (estSuivantDe(eExpressionLogiquePrime)) {
		//doz 7yd
	}
	else {
		syntaxError(eExpressionLogiquePrime);
	}
	xmlClose("expressionLogiquePrime");
}

void Syntaxique::expressionSimple() {
	xmlOpen("expressionSimple");
	if (estPremierDe(eTerme)) {
		expr = terme();
		string exprprime = expressionSimplePrime();
		if (expr != exprprime) //Verification de type
			semantique->logError("conflicting type");
	}
	else if (uniteCourante.UL == SOUS) {
		consommer(SOUS);
		expr = terme();
		string exprprime = expressionSimplePrime();
		if (expr != exprprime) 
			semantique->logError("conflicting type");
		generator->sous();
	}
	else {
		syntaxError(eExpression);
	}
	xmlClose("expressionSimple");
}

string Syntaxique::expressionSimplePrime() {
	int i = 0; string trm="";
	xmlOpen("expressionSimplePrime");
	if (uniteCourante.UL == ADD) {
		consommer(ADD);
		trm = terme(); 
		if (trm != "entier") 
			semantique->logError("conflicting type : expected type 'entier' ");
		expressionSimplePrime();
		generator->add();
	}
	else if (uniteCourante.UL == SOUS) {
		consommer(SOUS);
		trm = terme(); 
		if (trm != "entier") 
			semantique->logError("conflicting type : expected type 'entier' ");
		expressionSimplePrime();
		generator->sous();
	}
	else if (estSuivantDe(eExpressionSimplePrime)) {
		i++;
		//doz 7yd 
	}
	else {
		syntaxError(eExpressionSimplePrime);
	}
	xmlClose("expressionSimplePrime");
	if (i != 0) return expr;
	else return trm;
}

string Syntaxique::terme() {
	xmlOpen("terme");
	string trmprioritaire = "";
	if (estPremierDe(eTermePrioritaire)) {
		trmprioritaire = termePrioritaire();
		string trmprime = termePrime();
		if (trmprime != trmprioritaire && trmprime != "") 
			semantique->logError("conflicting type");
		
	}
	else {
		syntaxError(eTerme);
	}
	xmlClose("terme");
	return trmprioritaire;
}

string Syntaxique::termePrime() {
	xmlOpen("termePrime");
	string fac = "";
	if (uniteCourante.UL == MUL) {
		consommer(MUL);
		fac = facteur();
		termePrime();
		generator->mul();
		if (fac != "entier") 
			semantique->logError("conflicting type : expected type 'entier' ");
	}
	else if (uniteCourante.UL == DIV) {
		consommer(DIV);
		fac = facteur();
		termePrime();
		generator->div();
		if (fac != "entier") 
			semantique->logError("conflicting type : expected type 'entier' ");
	}
	else if (estSuivantDe(eTermePrime)) {
		//doz 7yd
	}
	else {
		syntaxError(eTermePrime);
	}
	xmlClose("termePrime");
	return fac;
}

string Syntaxique::termePrioritaire() {
	xmlOpen("termePrioritaire");
	string fac="";
	if (uniteCourante.UL == NON) {
		consommer(NON);
		fac = facteur();
		//generator.non();
	}
	else if (estPremierDe(eFacteur)) {
		fac = facteur();
	}
	else {
		syntaxError(eTermePrioritaire);
	}
	xmlClose("termePrioritaire");
	return fac;
}

string Syntaxique::facteur() {
	xmlOpen("facteur");
	string fac = "";
	if (estPremierDe(eIdentificateur)) {

		//semantique
		if (semantique->typeidentifTS(tmp) == "fct" )
			fct_tmp = tmp; 
		fac = semantique->typeidentifTS(tmp);

		//gen de code
		tmp = lexical->identifiants.get(uniteCourante.attribut); 
		variable data = semantique->getVariableData(tmp, ite_varlocalglobal);

		if (data.type == "entier" || data.type == "car") {
			if (data.scope == 0)//variable globale
				generator->empg(semantique->getVariableAddress(tmp, ite_varlocalglobal));
			else {
				generator->empl(semantique->getVariableAddress(tmp, ite_varlocalglobal));
			}
		}

		string tempIdent = lexical->identifiants.get(uniteCourante.attribut);
		identif();
		int identifType = facteurPrime(); // si c'est c'est une fonction ou un tableau => traitement special pour la gen de code

		//gen de code
		if (identifType == 1){ // une fonction
			generator->appel(tempIdent);
			int nbrParam = semantique->getVariableData(tempIdent, ite_varlocalglobal).param.size();
			generator->pile(-nbrParam);// on vire les parametres
		}
		else if (identifType == 2) { // un tableau
			//todo
		}
	}
	else if (estPremierDe(eCte)) {
		if (uniteCourante.UL == CONST) {
			fac = "entier";
		}
		generator->empc(uniteCourante.attribut);
		cte();
	}
	else if (uniteCourante.UL == PAROUV) {
		consommer(PAROUV);
		expression();
		if (uniteCourante.UL == PARFERM)
			consommer(PARFERM);
		else
			syntaxError(eFacteur);//à revoir
	}
	else if (uniteCourante.UL == QUOTE) {
		consommer(QUOTE);
		if (uniteCourante.UL == CAR) {
			consommer(CAR);
			fac = "car";
		}
		else syntaxError(eCaractere);
		consommer(QUOTE);
	}
	else {
		syntaxError(eFacteur);
	}
	xmlClose("facteur");
	return fac;
}

//Checks if the identif is an array or a function
//Returns 0 if none, 1 for function and 2 for array
int Syntaxique::facteurPrime() {
	int facteurType = 0;
	xmlOpen("facteurPrime");
	if (uniteCourante.UL == CROOUV) {
		facteurType = 2;
		consommer(CROOUV);
		if (semantique->VerifierTableau(tmp) == false) 
			semantique->logError("incompatible type: expected a table");
		expression();
		consommer(CROOUV);
	}
	else if (uniteCourante.UL == PAROUV) {
		facteurType = 1;
		consommer(PAROUV);
		
		//gen de code 
		generator->pile(1);// varaible resultat

		parametresEffectifs();

		//Actions semantiques
		if (semantique->VerifierFonction(fct_tmp, listeparam) == false) 
			semantique->logError("incompatible type: declaration of function incompatible");

		consommer(PARFERM);
	}
	else if (estSuivantDe(eFacteurPrime)) {
		//doz 7yd
	}
	else {
		syntaxError(eFacteurPrime);
	}
	xmlClose("facteurPrime");
	return facteurType;
}

void Syntaxique::parametresEffectifs() {
	xmlOpen("parametresEffectifs");
	if (estPremierDe(eExpressions)) {
		expressions();
	}
	else if (estSuivantDe(eParametresEffectifs)) {
		//doz 7yd
	}
	else {
		syntaxError(eParametresEffectifs);
	}
	xmlClose("parametresEffectifs");
}

void Syntaxique::expressions()
{
	xmlOpen("expressions");
	if (estPremierDe(eExpression)) {
		expression(); 
		listeparam.push_back(expr);
		expressionsPrime();
	}
	else {
		syntaxError(eExpression);
	}
	xmlClose("expressions");
}

void Syntaxique::expressionsPrime()
{
	xmlOpen("expressionsPrime");
	if (uniteCourante.UL == VIRG) {
		consommer(VIRG);
		expression(); 
		listeparam.push_back(expr);
		expressionPrime();
	}
	else if (estSuivantDe(eExpressionsPrime)) {
		//doz 7yd
	}
	else {
		syntaxError(eExpressionsPrime);
	}
	xmlClose("expressionsPrime");
}

void Syntaxique::operateurLogique()
{
	xmlOpen("operateurLogique");
	if (uniteCourante.UL == OU) {
		consommer(OU);
	}
	else if (uniteCourante.UL == ET)
		consommer(ET);
	else {
		syntaxError(eOperateurLogique);
	}
	xmlClose("operateurLogique");
}

string Syntaxique::comparaison()
{
	string comp = "";
	xmlOpen("comparaison");
	if (uniteCourante.UL == SUP) {
		consommer(SUP);
		comp = "SUP";
	}
	else if (uniteCourante.UL == INFEGAL) {
		consommer(INFEGAL);
		comp = "INFEGAL";
	}
	else if (uniteCourante.UL == SUPEGAL){
		consommer(SUPEGAL);
		comp = "SUPEGAL";
	}
	else if (uniteCourante.UL == INF) {
		consommer(INF);
		comp = "INF";
	}
	else if (uniteCourante.UL == EGALEGAL) {
		consommer(EGALEGAL);
		comp = "EGAL";
	}
	else
		syntaxError(eComparaison);
	xmlClose("comparaison");
	return comp;
}

void Syntaxique::identif()
{
	xmlOpen("identif");
	if (uniteCourante.UL == IDENT) {
		consommer(IDENT);
	}
	else
		syntaxError(eIdentificateur);
	xmlClose("identif");
}

void Syntaxique::cte()
{
	xmlOpen("cte");
	if (uniteCourante.UL == CONST) {
		consommer(CONST);
		//generator->empc(uniteCourante.attribut);
	}
	else
		syntaxError(eCte);
	xmlClose("cte");
}

//Methods

void Syntaxique::consommer(TUnite expected) {
	if (uniteCourante.UL != expected)
	{
		int line = lexical->getLine();
		int col = lexical->getColumn();

		auto itr = syntaxErrors.find(pair<int, int>(line, col));
		if (itr != syntaxErrors.end())//We already have some errors at this position
		{
			//Appending to existing errors
			itr->second.push_back(lexical->lexemeToString(uniteCourante));
		}
		else {
			//We add it to the errors map (tree)
			syntaxErrors.insert(pair< pair<int, int>, vector<string> >(pair<int, int>(line, col), vector<string>{lexical->lexemeToString(TUniteLexicale{ expected,0 })}));
		}

	}
	else {
		//Xml pour les terminaux
		ConsoleHandler::logDebug("Syntaxique", "Consumed " + lexical->lexemeToString(uniteCourante) + " at Ln" + to_string(lexical->getLine()) + ",Col " + to_string(lexical->getColumn()));
		xmlFile << expected << " "; //espace pour éviter de concatener deux terminaux consécutifs
		uniteCourante = lexical->uniteSuivante();
	}
	
}

void Syntaxique::syntaxError(Production prod) {
	vector<string> expectedSymbols;
	switch (prod)
	{
	case eProgramme:
	case eListeDeFonctions:
	case eListeDeDeclarations:
	case eDeclaration:
	case eDeclarations:
	case eListeParametres:
	case eParametres:
	case eParametre:
		expectedSymbols.push_back("type");
		break;
	case eFonction:
	case eDeclarationPrime:
	case eOperateurLogique:
	case eComparaison:
		expectedSymbols.push_back("logic operator");
		break;
	case eListeInstructions:
	case eInstruction:
	case eExpressionPrime:
	case eExpression:
	case eInstructionTriple:
	case eInstructionPrime:
		expectedSymbols.push_back("operator");
		break;
	case eDeclarationsPrime:
		expectedSymbols.push_back(",");
		break;
	case eDeclarationSeconde:
		expectedSymbols.push_back("[");
			break;
	case eExpressionLogique:
		expectedSymbols.push_back("logic expression");
		break;
	case eExpressionLogiquePrime:
		expectedSymbols.push_back("logic operator");
		break;
	case eExpressionSimple:
		expectedSymbols.push_back("constant");
		break;
	case eExpressionSimplePrime:
		expectedSymbols.push_back("arithmetic operator");
		break;
	case eTerme:
		expectedSymbols.push_back("constant");
		break;
	case eTermePrime:
		expectedSymbols.push_back("*"); expectedSymbols.push_back("/");
		break;
	case eTermePrioritaire:
		expectedSymbols.push_back("!"); expectedSymbols.push_back("(");
		break;
	case eFacteur:
		expectedSymbols.push_back("const");
		expectedSymbols.push_back("(");
		break;
	case eFacteurPrime:
		expectedSymbols.push_back("[");
		expectedSymbols.push_back("(");
		break;
	case eParametresEffectifs:
		expectedSymbols.push_back("*");
		expectedSymbols.push_back("/");
		break;
	case eExpressions:
		expectedSymbols.push_back("-"); 
		expectedSymbols.push_back("!"); 
		expectedSymbols.push_back("const");
		expectedSymbols.push_back("(");
		break;
	case eExpressionsPrime:
		expectedSymbols.push_back(",");
		break;

	case eIdentificateur:
		expectedSymbols.push_back("logic operator");
		break;
	case eCte:
		expectedSymbols.push_back("const");
		break;
	case eParametresPrime:
		expectedSymbols.push_back(",");
		break;
	case eInstructionSeconde:
		expectedSymbols.push_back("sinon");
		break;
	case eEnd:
			return;
	default://Is it necessary to handle all errors ? 
		//logError("Erreur de production " + to_string(prod) + " non gérée par le compilateur.");
		break;
	}
	int line = lexical->getLine();
	int col = lexical->getColumn();

	if (expectedSymbols.begin() != expectedSymbols.end()) {//If we found some errors
		auto itr = syntaxErrors.find(pair<int, int>(line, col));
		if (itr != syntaxErrors.end())//We already have some errors at this position
		{
			//Appending to existing errors
			itr->second.insert(itr->second.end(), expectedSymbols.begin(), expectedSymbols.end());
		}
		else {
			//We add it to the errors map (tree)
			syntaxErrors.insert(pair< pair<int, int>, vector<string> >(pair<int, int>(line, col), expectedSymbols));
		}
	}
}

//checks if the caracter is premier de l'unite en param
bool Syntaxique::estPremierDe(Production production) {
	switch (production)//TODO: eExpressionPrime
	{
		//outidrarine starts
	case eProgramme:
		return uniteCourante.UL == ENTIER || uniteCourante.UL == CAR;
		break;
	case eListeDeFonctions:
		return uniteCourante.UL == IDENT || uniteCourante.UL == MAIN;
		break;
	case eFonction:
		return uniteCourante.UL == IDENT;
		break;
	case eListeDeDeclarations:
		return uniteCourante.UL == ENTIER || uniteCourante.UL == CAR;
		break;
	case eDeclarations:
		return uniteCourante.UL == ENTIER || uniteCourante.UL == CAR;
		break;
	case eDeclarationsPrime:
		return uniteCourante.UL == VIRG;
		break;
	case eDeclaration:
		return uniteCourante.UL == ENTIER || uniteCourante.UL == CAR;
		break;
	case eDeclarationPrime:
		return uniteCourante.UL == VIRG;
		break;
	case eDeclarationSeconde:
		return uniteCourante.UL == CROOUV;
		break;
	case eListeParametres:
		return uniteCourante.UL == ENTIER || uniteCourante.UL == CAR;
		break;
		//outidrarine finishes

		//souhail starts
	case eExpressionLogique:
		return estPremierDe(eExpressionSimple) == true;
		break;
	case eExpressionLogiquePrime:
		return estPremierDe(eComparaison) == true || estSuivantDe(production);;
		break;
	case eExpressionSimple:
		return uniteCourante.UL == CONST || uniteCourante.UL == CONSTCAR || uniteCourante.UL == PAROUV || uniteCourante.UL == QUOTE
			|| uniteCourante.UL == NON || uniteCourante.UL == SOUS || uniteCourante.UL == IDENT;
		break;
	case eExpressionSimplePrime:
		return uniteCourante.UL == ADD || uniteCourante.UL == SOUS || estSuivantDe(production);
		break;
	case eTerme:
		return estPremierDe(eFacteur);
		break;
	case eTermePrime:
		return uniteCourante.UL == MUL || uniteCourante.UL == DIV || estSuivantDe(production);
		break;
	case eTermePrioritaire:
		return estPremierDe(eTerme) || uniteCourante.UL == NON;
		break;
	case eFacteur:
		return uniteCourante.UL == CONST || uniteCourante.UL == CONSTCAR || uniteCourante.UL == PAROUV || uniteCourante.UL == QUOTE
			|| uniteCourante.UL == IDENT;
		break;
	case eFacteurPrime:
		return uniteCourante.UL == PAROUV || uniteCourante.UL == CROOUV || estSuivantDe(production);
		break;
		//souhail finishes
	//begin zac
	case eParametresEffectifs:
		return uniteCourante.UL == MUL || uniteCourante.UL == DIV || estSuivantDe(production);
		break;
	case eExpressions: //todo
		return estPremierDe(eExpression);
		break;
	case eExpressionsPrime:
		return uniteCourante.UL == VIRG || estSuivantDe(production);;
		break;
	case eOperateurLogique:
		return uniteCourante.UL == OU || uniteCourante.UL == ET;
	case eComparaison:
		return uniteCourante.UL == INF || uniteCourante.UL == SUP || uniteCourante.UL == INFEGAL
			|| uniteCourante.UL == EGALEGAL;
	case eIdentificateur:
		return uniteCourante.UL == IDENT;
	case eCte:
		return uniteCourante.UL == CONST;
	case eExpression:
		return estPremierDe(eExpressionSimple);
	case eExpressionPrime:
		return uniteCourante.UL == ET || uniteCourante.UL == OU || estSuivantDe(production);
		//end zac

		// start testoxe
	case eParametres:
		return uniteCourante.attribut == ENTIER || uniteCourante.UL == CAR || estSuivantDe(eParametres);
		break;
	case eParametresPrime:
		return uniteCourante.UL == VIRG;
		break;
	case eParametre:
		return uniteCourante.UL == ENTIER || uniteCourante.UL == CAR;
		break;
	case eListeInstructions:
		return estSuivantDe(eListeInstructions) || estSuivantDe(eExpression) || estPremierDe(eExpression) || uniteCourante.UL == IDENT
			|| uniteCourante.UL == RETOUR || uniteCourante.UL == SI || uniteCourante.UL == TANTQUE || uniteCourante.UL == MOTCLE;
		break;
	case eInstruction:
		return estPremierDe(eExpression) || uniteCourante.UL == IDENT || uniteCourante.UL == RETOUR || uniteCourante.UL == MOTCLE
			|| uniteCourante.UL == SI || uniteCourante.UL == TANTQUE || uniteCourante.UL == LIRE || uniteCourante.UL == ECRIRE;
		break;
	case eInstructionPrime:
		return uniteCourante.UL == EGAL || uniteCourante.UL == CROOUV;
		break;
	case eInstructionSeconde:
		return uniteCourante.UL == SINON || estSuivantDe(production);
		break;
	case eInstructionTriple:
		return estPremierDe(eExpression) || uniteCourante.UL == LIRE;
		break;

	default:
		cout << "Production non reconnue" << endl;
		break;
	}
	return false;
}

bool Syntaxique::estSuivantDe(Production production) {
	switch (production)
	{
		//outidrarine starts
	case eProgramme:
		return uniteCourante.UL == END;
		break;
	case eListeDeFonctions:
		return uniteCourante.UL == END;
		break;
	case eFonction:
		return uniteCourante.UL == IDENT || uniteCourante.UL == MAIN;
		break;
	case eListeDeDeclarations:
		return uniteCourante.UL == IDENT || uniteCourante.UL == MAIN;
		break;
	case eDeclarations:
		return uniteCourante.UL == PTVRG;
		break;
	case eDeclarationsPrime:
		return uniteCourante.UL == VIRG || uniteCourante.UL == PTVRG;
		break;
	case eDeclaration:
		return uniteCourante.UL == VIRG || uniteCourante.UL == PTVRG;
		break;
	case eDeclarationPrime:
		return uniteCourante.UL == VIRG || uniteCourante.UL == PTVRG;
		break;
	case eDeclarationSeconde:
		return uniteCourante.UL == VIRG || uniteCourante.UL == PTVRG;
		break;
	case eListeParametres:
		return uniteCourante.UL == PARFERM;
		break;
		//outidrarine finishes

		//souhail starts
	case eExpressionLogique:
		return estSuivantDe(eExpression) == true || uniteCourante.UL == ET || uniteCourante.UL == OU;
		break;
	case eExpressionLogiquePrime:
		return estSuivantDe(eExpressionLogique) == true;
		break;
	case eExpressionSimple:
		return estSuivantDe(eExpressionLogique) == true || estPremierDe(eComparaison) == true;
		break;
	case eExpressionSimplePrime:
		return estSuivantDe(eExpressionSimple) == true;
		break;
	case eTerme:
		return estSuivantDe(eExpressionSimple) == true || uniteCourante.UL == ADD || uniteCourante.UL == SOUS;
		break;
	case eTermePrime:
		return estSuivantDe(eTerme) == true;
		break;
	case eTermePrioritaire:
		return estSuivantDe(eTerme) == true || uniteCourante.UL == MUL || uniteCourante.UL == DIV;
		break;
	case eFacteur:
		return estSuivantDe(eTermePrioritaire) == true;
		break;
	case eFacteurPrime:
		return estSuivantDe(eTermePrioritaire) == true;
		break;
	case eParametresEffectifs:
		return uniteCourante.UL == PARFERM;
		break;
		//souhail finishes
	//begin zac
	case eExpressions:
		return uniteCourante.UL == PARFERM;
		break;
	case eExpressionsPrime:
		return uniteCourante.UL == PARFERM;
		break;
	case eOperateurLogique:
		return estPremierDe(eExpressionLogique);
	case eComparaison:
		return uniteCourante.UL == IDENT || uniteCourante.UL == CONST || uniteCourante.UL == CAR
			|| uniteCourante.UL == PAROUV || uniteCourante.UL == NON || uniteCourante.UL == SOUS;
	case eIdentificateur:
		return estPremierDe(eFacteur) || estSuivantDe(eFacteur) || estPremierDe(eInstructionPrime)
			|| estSuivantDe(eParametre) || estSuivantDe(eDeclarationSeconde) || uniteCourante.UL == PAROUV;
	case eCte:
		return estSuivantDe(eFacteur);
		//end zac

		//start testoxe
	case eParametres:
		return uniteCourante.UL == PARFERM;
		break;
	case eParametresPrime:
		return uniteCourante.UL == PARFERM;
		break;
	case eParametre:
		return uniteCourante.UL == VIRG || uniteCourante.UL == PARFERM;
		break;
	case eListeInstructions:
		return uniteCourante.UL == ACCFERM;
		break;
	case eInstruction:
		return uniteCourante.UL == PTVRG;
		break;
	case eInstructionPrime:
		return uniteCourante.UL == PTVRG;
		break;
	case eInstructionSeconde:
		return uniteCourante.UL == PTVRG;
		break;
	case eInstructionTriple:
		return uniteCourante.UL == PTVRG;
		break;
	case eExpression:
		return uniteCourante.UL == VIRG || uniteCourante.UL == ALORS || uniteCourante.UL == FAIRE || uniteCourante.UL == CROFER
			|| uniteCourante.UL == ACCFERM || uniteCourante.UL == PTVRG || uniteCourante.UL == PARFERM;
	case eExpressionPrime:
		return uniteCourante.UL == VIRG || uniteCourante.UL == ALORS || uniteCourante.UL == FAIRE || uniteCourante.UL == CROFER
			|| uniteCourante.UL == ACCFERM || uniteCourante.UL == PTVRG || uniteCourante.UL == PARFERM;
	default:
		logError("Production non reconnue");
		break;
	}
	return false;
}

void Syntaxique::logDebug(string message)
{
	if (lexical->enableDebug)//ugly but doing fine
		ConsoleHandler::logDebug("Syntaxique" , message);
}

void Syntaxique::logError(string error)
{
	ConsoleHandler::logError("Syntaxique",error);
}


//Destructeur
Syntaxique::~Syntaxique()
{
	end();
}