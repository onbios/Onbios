/*
Basic Code for the Minispectro
Copyright (C) 2018 Silvestre Perret, Arnaud Bauer, Guillaume Dominici

This program is free software under the terms of the GPL-2.0 Licence 
(https://github.com/onbios/Onbios/blob/master/LICENSE)
*/

/*
  
  The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)

 http://www.arduino.cc/en/Tutorial/LiquidCrystalHelloWorld

*/

//appel de la bibliotheque du capteur
#include "AS726X.h" 
//appel de la bibliotheque cristaux liquide pour l'ecran          
#include <LiquidCrystal.h>    

//creation de l'objet sensor de classe AS726X
AS726X sensor;

//******************* DEFINITION DES CONSTANTES *********************************************************************************************************

//coefficient directeur par canal (corrections pour obtenir les memes valeurs d'absorbance qu'avec un spectro de laboratoire)
float coefViolet = 0.998 ;
float coefBlue = 0.912 ;
float coefGreen = 0.593 ;
float coefYellow = 0.554 ;
float coefOrange = 1.02 ;
float coefRed = 0.963 ;

//Definition des longueurs d'onde pour l'affichage des resultats sur l'ecran
float coefficients [6] = {0.998, 0.912, 0.593, 0.554, 1.02, 0.963} ;
int waveLengths [6] = {450, 500, 550, 570, 600, 650} ;
String colors [6] = {"V", "B", "G", "Y", "O", "R"} ;

//definition de parametres lies a la mesure
float changeAbsThreshold = 0.4 ;   // seuil pour la détection d'insertion de cuve valeur initiale 0.4
int heatingDuration = 120 ; // en secondes
long timeLedOff = 28L * 1000L ; // en millisecondes. "L" signifie que la variable est de type 'long'
long timeLedOn = 8L * 1000L ;   // en millisecondes. "L" signifie que la variable est de type 'long'
long timestamp;                 // délai pour la détection de cuve
unsigned long msecondes ;       // en millisecondes variable pour l'affichage du temps sur le port serie
 
// definition des variables contenant des mesures
float intensities [6] ;   // intensite lumineuse mesuree par le capteur. Tableau contenant les valeurs aux 6 longueurs d'onde.
float blankIntensities [6]; // Tableau des intensite mesurees sur le blanc.
float absIntensities [6]; // Tableau des absorbances aux 6 longueurs d'onde.

//definition des pins hors ecran LCD
const int ledPin = 7 ;
bool ledOn = false ;

//definition des pins de l'ecran LCD pour la connexion avec la carte Arduino
const int rs = 12,
          en = 11,
          d4 = 5,
          d5 = 4,
          d6 = 3,
          d7 = 2;

//definition du contraste de l'ecran LCD
const int contrastPin = 9 ;
int contrastValue = 100 ; 
          
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);  



//******************* SETUP n'est effectue qu'une seule fois **************************************************************************************//   
           
void setup() {
  // Initialisation de la carte Arduino, du capteur et de l'écran.
  Serial.begin(115200) ;    // Mise en route du port Serie (communication avec l'ordinateur). On precise la vitesse de communication
  pinMode(ledPin,OUTPUT) ;  // mode de la pin a laquelle est connectee la led en OUTPUT
  switchOnLed() ;           // on allume la LED
  sensor.begin() ;          // mise en route du capteur           
  pinMode(contrastPin,OUTPUT) ;     // mode de la pin 9 utilisee pour regler le contraste de l'ecran en OUTPUT   
  analogWrite(contrastPin,contrastValue) ;  //  application de la valeur contrastValue définie plus haut a la pin 9  
  lcd.begin(16, 2) ;        // mise en route de l'ecran LCD, 16 caracteres, 2 lignes   
  lcd.print("Initialisation...") ;  //affichage d'un message sur l'ecran            
  delay(2000) ;             // mise en pause du programme pendant 2 secondes pour permettre la lecture     
  lcd.clear() ;             // on efface l'écran
  
  // On prechauffe la Led (fonctions definies plus bas).
  preheat() ;
  // On fait le blanc
  takeBlank() ;
} 


//******************* LOOP s'execute en boucle en permanence ******************************************************************************************//

void loop() {
  //on utilise la fonction priseMesure pour prendre les mesures d'intensite lumineuse aux 6 longueurs d'onde
  takeMeasure() ;
  //on utilise la fonction calculAbsorbance pour calculer les absorbances
  computeAbsorbance(intensities, blankIntensities) ;
  //on utilise la fonction printResultsLCD pour afficher les resultats sur l'ecran LCD
  printResultsLCD() ;
}


//******************* DEFINITION DES FONCTIONS UTILISEES ***************************************************************************************************//


//la fonction takeMeasure enregistre les intensites lumineuses mesurees a chaque longueur d'onde

void takeMeasure() {
  lcd.clear() ;                 // on efface ce qu'il pouvait rester sur l'ecran.
  lcd.setCursor(0, 0) ;         // on indique au programme qu'il faudra ecrire en haut a gauche de l'ecran
  switchOnLed();                // on allume la LED
  lcd.print("Mesure en cours") ; 
  
  sensor.takeMeasurements() ;   // le capteur prend les mesures et les met en memoire
  sensor.printMeasurements() ;  // puis a la meme ligne, on affiche les intensites mesurees sur le port serie
  switchOffLed() ;              // on eteint la LED
  msecondes = millis();         // on affecte le nombre de millisecondes ecoulees depuis la mise en route (obtenues par la fonction millis()) à la variable msecondes
  Serial.print(" Temps (s): "); // on affiche sur le port série "Temps (s) :"
  Serial.println(msecondes) ;   // on affiche sur le port série à la même ligne la variable msecondes
  
  //on affecte les valeurs de chaque longueur d'onde aux 6 variables associees
  intensities [0] = sensor.getCalibratedViolet() ;  // Le capteur va chercher les mesures mises en mémoire avec sensor.takeMeasurements(). La variable intensities[0] est affectée de cette valeur.
  intensities [1] = sensor.getCalibratedBlue() ;
  intensities [2] = sensor.getCalibratedGreen() ;
  intensities [3] = sensor.getCalibratedYellow() ;
  intensities [4] = sensor.getCalibratedOrange() ;
  intensities [5] = sensor.getCalibratedRed() ;
  delay(100);                  // on attend 100ms
  lcd.clear() ;                // on efface l'ecran
}

//------------------------------------------------------------------------------------------------------------

//la fonction takeSimpleMeasure prend une mesure avec possibilité d'utiliser la petite LED du capteur (différente de la LED blanche en face du capteur). Utile pour detection de cuve.

void takeSimpleMeasure(bool withBulb){                      // la fonction prend en argument un boolee. True si la LED du capteur est allumee pendant la mesure, False sinon.
  //le capteur prend les mesures et les met en memoire
  if (withBulb) {sensor.takeMeasurementsWithBulb() ;}
  else {sensor.takeMeasurements() ;}
  //on affecte les valeurs de chacun des canaux aux 6 variables associees
  intensities [0] = sensor.getCalibratedViolet() ;
  intensities [1] = sensor.getCalibratedBlue() ;
  intensities [2] = sensor.getCalibratedGreen() ;
  intensities [3] = sensor.getCalibratedYellow() ;
  intensities [4] = sensor.getCalibratedOrange() ;
  intensities [5] = sensor.getCalibratedRed() ;
}

//------------------------------------------------------------------------------------------------------------

// la fonction takeBlank permet de faire le blanc avec detection automatique de la cuve

void takeBlank() {
  lcd.clear() ;                    // on efface l'ecran LCD
  lcd.print("Inserez BLANC") ;     // on affiche "Inserez BLANC" sur l'ecran LCD
  //sensor.takeMeasurementsWithBulb() ;  //ligne que j'ai ajoutée
  sensor.printMeasurements() ;     // on affiche les intensites lumineuses sur le port serie
  detectCuvetteInsertion() ;       // on fait appel a la fonction detection de cuvette
  switchOffLed() ;                 // on eteint la LED blanche
  delay(1000) ;                    // on attend 1 seconde
  lcd.clear() ;                    // on efface l'ecran LCD
  lcd.print("BLANC detecte") ;     // on affiche "Blanc detecte" sur l'ecran LCD
  delay(4000) ;                    // on attend 4s
  lcd.clear() ;                    // on efface l'ecran LCD
  lcd.print("Mesure blanc en cours") ;          // on affiche sur l'ecran "Mesure blanc en cours"
  // On fait la mesure ici :
  takeMeasure();
  for (int k=0; k < 6; k++) {
     blankIntensities [k] = intensities [k] ;  // on affecte a chaque case du tableau blankIntensities les valeurs d'intensite lumineuses mesurees a chaque longueur d'onde
  }
  Serial.print(" Blank: ") ;                   // on affiche "Blank : " sur le port serie de l'ordinateur
  printIntensities(blankIntensities) ;         // puis on affiche les intensites lumineuses du blanc sur le port serie

  lcd.clear() ;                    // on efface l'ecran LCD
  lcd.print("BLANC OK") ;          // on affiche "BLANC ok" sur l'ecran
  delay(3000) ;                    // on attend 3s pour laisser le temps de lire l'indication
  
  
  lcd.clear() ;                    
  lcd.print("Inserez CUVE") ;      
  detectCuvetteInsertion() ;       // on fait appel a la fonction de detection de la cuve
  switchOffLed() ;                 // on fait appel a la fonction qui eteint la LED blanche
  delay(1000);
  lcd.clear() ;
  lcd.print("CUVE detectee") ;
  delay(5000) ;
  lcd.clear() ;
}

//------------------------------------------------------------------------------------------------------------

// la fonction preheat permet de prechauffer la LED

void preheat() {
  //boucle de prechauffage
  for(int k = 0; k < heatingDuration; k++) {                   
    if(ledOn && (millis() - timestamp > timeLedOn)) { 
      switchOffLed() ;
      timestamp = millis() ;
    }
    else if ((not ledOn) && (millis() - timestamp > timeLedOff)) {
      switchOnLed() ;
      timestamp = millis() ;
    }              
    lcd.clear() ;                 //on efface 
    lcd.setCursor(4,0) ;          //on place le "curseur" sur le premier caractere de la premiere ligne de l'ecran
    lcd.print("Prechauffage") ;   //on ecrit prechauffage sur l'ecran
    int cursorX = 14 ;
    if (heatingDuration - k >= 10) {cursorX -= 1 ;}        // on ajuste la position du curseur selon la taille du texte à afficher
    if (heatingDuration - k >= 100) {cursorX -= 1 ;}       // on ajuste la position du curseur selon la taille du texte à afficher
    lcd.setCursor(cursorX,1) ;
    lcd.print(String(heatingDuration - k) + "s") ;         //on affiche la duree restante sur l'ecran :
    takeSimpleMeasure(false) ;                             //le capteur prend les mesures avec la LED blanche et les garde en memoire
    delay(500) ;
  }
  lcd.clear() ;
}

///------------------------------------------------------------------------------------------------------------

//la fonction computeAbsorbance permet de calculer l'absorbance à partir des intensités luminseuses mesurées

void computeAbsorbance(float * newIntensities, float * refIntensities) {
  for (int k=0; k < 6; k++) {
     absIntensities [k] = log10(refIntensities[k] / newIntensities[k]) / coefficients[k] ;        // on enregistre les valeurs calculées dans un tableau à 6 cases (pour 6 longueurs d'onde)
  }
  Serial.print(" Abs: ") ;                    // on affiche la valeur d'absorbance sur le port série, avec du texte pour faciliter la lecture
  printIntensities(absIntensities) ;
}


//------------------------------------------------------------------------------------------------------------

// detection Insertion Cuve = detection d'un valeur seuil d'absorbance entre deux mesures consecutives

void detectCuvetteInsertion() {
  bool cuvetteInserted = false ;
  bool initialized = false;
  float changeAbsSum = 0.0 ;
  float refIntensities [6] ;
    
  while (not cuvetteInserted) {
    if(ledOn && (millis() - timestamp > timeLedOn)) { 
      switchOffLed() ;
      takeSimpleMeasure(true) ; // with sensorLed ON
    }
    else if ((not ledOn) && (millis() - timestamp > timeLedOff)) {
      switchOnLed() ;
      takeSimpleMeasure(true) ; // with sensorLed ON
    }

    for (int k=0; k<6; k++) {
      refIntensities[k] = intensities[k] ;
    }
    takeSimpleMeasure(true) ; // with sensorLed ON
    computeAbsorbance(intensities, refIntensities) ;
  
    changeAbsSum = 0.0 ;
    for (int k=0; k<6;k++) {changeAbsSum += absIntensities[k] ;}
    Serial.println(changeAbsSum) ;
    if (initialized && changeAbsSum < -1*changeAbsThreshold) {  //à la base changeAbsSum < -1*changeAbsThreshold
      Serial.println(" Cuvette detected") ;
      cuvetteInserted = true ;
    }
    initialized = true;
  }
  
}

//------------------------------------------------------------------------------------------------------------

// la fonction printResultsLCD permet d'afficher les resultats sur l'écran

void printResultsLCD() {
  lcd.clear() ;                   // on commence par effacer ce qu'il y avait sur l'écran
  // pour chacune des 6 longueurs d'onde, on affiche le résultat sur l'écran. Par ex "Absorbance 550nm 0.63", puis on efface l'écran et on passe à la longueur d'onde suivante
  for (int k=0; k < 6; k++) {     
    lcd.print("Absorbance " + String(waveLengths[k]) + "nm") ;
    int cursorX = 12 ;            // on ajuste la position du curseur sur l'ecran en fonction de la taille du resultat a afficher
    if (absIntensities[k] < 0) {cursorX -= 1;}
    if (abs(absIntensities[k]) > 10) {cursorX -= 1;}
    lcd.setCursor(cursorX, 1) ;
    lcd.print(absIntensities[k]);
    delay(4000) ;                 // on laisse le resultat de chaque longueur d'onde affiche 4 secondes
    lcd.clear() ;                 // on efface ce qui est affiche sur l'ecran
  }
}


//------------------------------------------------------------------------------------------------------------

// La fonction suivante permet d'afficher les 6 intensités sur le port série de l'ordinateur
void printIntensities(float * intensitiesToPrint) {
  for (int k=0; k < 6; k++) {
     Serial.print(colors[k] + "[" + String(intensitiesToPrint[k]) + "] ") ;
  }
  Serial.println("");
}

//------------------------------------------------------------------------------------------------------------

// La fonction suivante permet d'allumer la LED blanche
void switchOnLed() {
  //on applique un potentiel de 5V sur la pin 7
  digitalWrite(7,HIGH) ;
  ledOn = true ;
  timestamp = millis() ;
}

//------------------------------------------------------------------------------------------------------------

// La fonction suivante permet d'éteindre la LED blanche
void switchOffLed() {
  //on applique un potentiel de 0V sur la pin 7
  digitalWrite(7,LOW) ;
  ledOn = false ;
  timestamp = millis() ;
}


.
