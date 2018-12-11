
//*********************APPEL DES BIBLIOTHEQUES*********************************************************************

#include <Wire.h>                 //appel de la bibliotheque permettant de gérer les protocoles I2C

#include <LiquidCrystal_I2C.h>    //appel de la bibliotheque cristaux liquide pour l'ecran  

#include "AS726X.h"               //appel de la bibliotheque du capteur             

AS726X sensor;                    //creation de l'objet sensor de classe AS726X

LiquidCrystal_I2C lcd(0x3F,20,4); //creation de l'objet lcd de classe LiquidCrystal_I2C

//******************* DEFINITION DES CONSTANTES *********************************************************************************************************


//Definition des longueurs d'onde pour l'affichage des resultats sur l'ecran
float coefficients [6] = {1, 1, 1, 1, 1, 1} ;
int waveLengths [6] = {450, 500, 550, 570, 600, 650} ;
String colors [6] = {"V", "B", "G", "Y", "O", "R"} ;

//definition de parametres lies a la mesure
float changeAbsThreshold = 0.1 ;   // seuil pour la détection d'insertion de cuve valeur initiale 0.4
int heatingDuration = 120 ;       // la fonction preheat n'est à présent plus utilisée
int intensiteSeuil = 5000 ;               //Seuil d'intensité pour la LED
long timeLedOff = 24L * 1000L ; // en millisecondes. "L" signifie que la variable est de type 'long'
long timeLedOn = 1L * 1000L ;   // en millisecondes. "L" signifie que la variable est de type 'long'
long timestamp;                 // délai pour la détection de cuve
unsigned long msecondes ;       // en millisecondes variable pour l'affichage du temps sur le port serie
 
// definition des variables contenant des mesures
float intensities [6] ;   // intensite lumineuse mesuree par le capteur. Tableau contenant les valeurs aux 6 longueurs d'onde.
float blankIntensities [6]; // Tableau des intensite mesurees sur le blanc.
float absIntensities [6]; // Tableau des absorbances aux 6 longueurs d'onde.

//definition des pins hors ecran LCD
const int ledPin = 3 ;    //on se place sur un port PWM
int ledIntensity = 1 ;     //intensité de la LED à calibrer
bool ledOn = false ;      // etat de la LED, quand ledOn = false, la l'état de la LED est éteint

bool modeViolet = false ;   //booléens utilisés pour la fonction de changement de mode
bool modeBleu = false ;
bool modeVert = false ;
bool modeJaune = false ;
bool modeOrange = false ;
bool modeRouge = false ;



//******************* SETUP n'est effectue qu'une seule fois **************************************************************************************//   
           
void setup() {
  delay(1000);
  // Initialisation de la carte Arduino, du capteur et de l'écran.
  Serial.begin(115200) ;    // Mise en route du port Serie (communication avec l'ordinateur). On precise la vitesse de communication
  pinMode(ledPin,OUTPUT) ;  // mode de la pin a laquelle est connectee la led en OUTPUT
  //switchOnLed() ;           // on allume la LED
  sensor.begin() ;          // mise en route du capteur    
  sensor.setIntegrationTime(255) ;       
  //pinMode(contrastPin,OUTPUT) ;     // mode de la pin 9 utilisee pour regler le contraste de l'ecran en OUTPUT   
  //analogWrite(contrastPin,contrastValue) ;  //  application de la valeur contrastValue définie plus haut a la pin 9  
  //lcd.begin(16, 2) ;        // mise en route de l'ecran LCD, 16 caracteres, 2 lignes   
  lcd.init() ;
  lcd.backlight() ;

  switchOnLed() ;
  sensor.takeMeasurements() ;
  while (sensor.getCalibratedViolet() < 1000) {
    lcd.setCursor(0,0) ;
    lcd.print("Retirer Mode") ;
    sensor.takeMeasurements() ;
    delay(1000) ;
  }

  lcd.clear() ;
  switchOffLed() ;
  
  lcd.print("Initialisation...") ;  //affichage d'un message sur l'ecran            
  delay(2000) ;             // mise en pause du programme pendant 2 secondes pour permettre la lecture   
  //insérer ici la fonction de choix du mode
  ledCalibration() ;
  lcd.clear() ;             // on efface l'écran
  // On prechauffe la Led (fonctions definies plus bas).
  //preheat() ;             // la fonction preheat a été rendue obsolète et n'est plus utilisée ce qui permet un démarrage immédiat de la machine
  // On fait le blanc
  takeBlank() ;
} 


//******************* LOOP s'execute en boucle en permanence ******************************************************************************************//

void loop() {
  //on utilise la fonction priseMesure pour prendre les mesures d'intensite lumineuse aux 6 longueurs d'onde
  takeMeasure() ;
  //on utilise la fonction calculAbsorbance pour calculer les absorbances
  if (intensities[0] == 0) {      // dans le cas ou la cuve opaque est insérée, les intensités lues par le capteur sont nulles, on rentre dans la phase de changement de longueur d'onde
    chooseMode() ;                // fonction qui permet de selectionner la longueur d'onde de travail
    takeMeasure() ;               // prise de mesure
  }
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

  lcd.clear() ;
  
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
      Serial.println(ledOn) ;
    }
    else if ((not ledOn) && (millis() - timestamp > timeLedOff)) {
      switchOnLed() ;
      takeSimpleMeasure(true) ; // with sensorLed ON
      Serial.println(ledOn) ;
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
  int cursorX = 11 ;            // on ajuste la position du curseur sur l'ecran en fonction de la taille du resultat a afficher
    
  // pour chacune des 6 longueurs d'onde, on affiche le résultat sur l'écran. Par ex "Absorbance 550nm 0.63", puis on efface l'écran et on passe à la longueur d'onde suivante
  if (modeViolet == true) {
    lcd.print("Absorbance " + String(waveLengths[0]) + "nm") ;
    if (absIntensities[0] < 0) {cursorX -= 1;}
    if (abs(absIntensities[0]) > 10) {cursorX -= 1;}
    lcd.setCursor(cursorX, 1) ;
    lcd.print(absIntensities[0], 3);
    delay(8000) ;                 // on laisse le resultat de chaque longueur d'onde affiche 4 secondes
    lcd.clear() ;  
  }

  if (modeBleu == true) {
    lcd.print("Absorbance " + String(waveLengths[1]) + "nm") ;
    if (absIntensities[1] < 0) {cursorX -= 1;}
    if (abs(absIntensities[1]) > 10) {cursorX -= 1;}
    lcd.setCursor(cursorX, 1) ;
    lcd.print(absIntensities[1], 3);
    delay(8000) ;                 // on laisse le resultat de chaque longueur d'onde affiche 4 secondes
    lcd.clear() ;  
  }

  if (modeVert == true) {
    lcd.print("Absorbance " + String(waveLengths[2]) + "nm") ;
    if (absIntensities[2] < 0) {cursorX -= 1;}
    if (abs(absIntensities[2]) > 10) {cursorX -= 1;}
    lcd.setCursor(cursorX, 1) ;
    lcd.print(absIntensities[2], 3);
    delay(8000) ;                 // on laisse le resultat de chaque longueur d'onde affiche 4 secondes
    lcd.clear() ;  
  }

  if (modeJaune == true) {
    lcd.print("Absorbance " + String(waveLengths[3]) + "nm") ;
    if (absIntensities[3] < 0) {cursorX -= 1;}
    if (abs(absIntensities[3]) > 10) {cursorX -= 1;}
    lcd.setCursor(cursorX, 1) ;
    lcd.print(absIntensities[3], 3);
    delay(8000) ;                 // on laisse le resultat de chaque longueur d'onde affiche 4 secondes
    lcd.clear() ;  
  }

  if (modeOrange == true) {
    lcd.print("Absorbance " + String(waveLengths[4]) + "nm") ;
    if (absIntensities[4] < 0) {cursorX -= 1;}
    if (abs(absIntensities[4]) > 10) {cursorX -= 1;}
    lcd.setCursor(cursorX, 1) ;
    lcd.print(absIntensities[4], 3);
    delay(8000) ;                 // on laisse le resultat de chaque longueur d'onde affiche 4 secondes
    lcd.clear() ;  
  }

  if (modeRouge == true) {
    lcd.print("Absorbance " + String(waveLengths[5]) + "nm") ;
    if (absIntensities[5] < 0) {cursorX -= 1;}
    if (abs(absIntensities[5]) > 10) {cursorX -= 1;}
    lcd.setCursor(cursorX, 1) ;
    lcd.print(absIntensities[5], 3);
    delay(8000) ;                 // on laisse le resultat de chaque longueur d'onde affiche 4 secondes
    lcd.clear() ;  
  }
  if (modeViolet == false && modeBleu == false && modeVert == false && modeJaune == false && modeOrange == false && modeRouge==false) {
    for (int k=0; k < 6; k++) {     
      lcd.print("Absorbance " + String(waveLengths[k]) + "nm") ;
      if (absIntensities[k] < 0) {cursorX -= 1;}
      if (abs(absIntensities[k]) > 10) {cursorX -= 1;}
      lcd.setCursor(cursorX, 1) ;
      lcd.print(absIntensities[k], 3);
      delay(4000) ;                 // on laisse le resultat de chaque longueur d'onde affiche 4 secondes
      lcd.clear() ;                 // on efface ce qui est affiche sur l'ecran
    }
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
  analogWrite(ledPin,ledIntensity) ; // on applique une tension de 5V  sur la pin de la LED, à une fréquence qui est égale à ledIntensity
  ledOn = true ;                     // l'état de la led passe à true : la LED est allumée
  timestamp = millis() ;            
}

//------------------------------------------------------------------------------------------------------------

// La fonction suivante permet d'éteindre la LED blanche
void switchOffLed() {
  //on applique un potentiel de 0V sur la pin 7
  analogWrite(ledPin,0) ;           // on applique une tension nulle surla pin de la LED 
  ledOn = false ;                   // l'état de la LED passe à false : la LED est éteinte
  timestamp = millis() ;
}

void chooseMode() {         // Cette fonction permet de choisir la longueur d'onde sur laquelle la machine fonctionne en insérant une cuve opaque.
  modeViolet = false ;      //modeViolet = false signifie que pour le moment la longueur d'onde 450nm n'a pas été sélectionnée
  modeBleu = false ;        //idem pour les autres longueurs d'onde
  modeVert = false ;
  modeJaune = false ;
  modeOrange = false ;
  modeRouge = false ;
  
                          
  lcd.print("Choix de lambda") ;    // On affiche "Choix du mode à l'écran durant 3s
  delay(3000) ;
  lcd.clear() ;
  long previousMillis = millis() ;
    
    while (millis() - previousMillis < 4000) {      //Pendant trois secondes on attend que l'on retire la cuve noire
      lcd.print("Choisir lambda") ;                 //On affiche "Choisir lambda = 450nm?" à l'écran
      lcd.setCursor(0,1) ;
      lcd.print("= 450nm?") ;
      switchOnLed() ;                               //On allume la LED
      delay(3990) ;                                 //On attend 3990ms pour prendre la mesure suivante, cela permet de laisser l'information affichée à l'écran et au manipulateur de retirer la cuve opaque s'il souhaite choisir cette longueur d'onde
      sensor.takeMeasurements() ;                   //On prend une mesure
      lcd.clear();                                  //On efface les informations affichées à l'écran
      
      if (sensor.getCalibratedViolet() != 0) {      //Si cette mesure comporte un violet non nul (pourrait fonctionner avec les autres canaux) cela veut dire que la cuve opaque a été retirée et que l'utilisateur veut choisir cette longueur d'onde
        lcd.clear() ;
        lcd.print("Reglage : 450nm") ; 
        delay(4000) ;
        modeViolet = true ;                         //La longueur d'onde 450nm a été choisie
        return ;   
      }
      switchOffLed() ;      
    }
    previousMillis = millis() ;

    while (millis() - previousMillis < 4000) {      //Pendant quatre secondes on attend que l'on retire la cuve noire
      lcd.print("Choisir lambda") ;
      lcd.setCursor(0,1) ;
      lcd.print("= 500nm?") ;
      switchOnLed() ;
      delay(3990) ;
      sensor.takeMeasurements() ;
      lcd.clear() ;
      
      if (sensor.getCalibratedViolet() != 0) {
        lcd.clear() ;
        lcd.print("Reglage : 500nm") ; 
        delay(4000) ;
        modeBleu = true ;  
        return ;
      }
      switchOffLed() ;          
    }
    previousMillis = millis() ;

    while (millis() - previousMillis < 4000) {      //Pendant quatre secondes on attend que l'on retire la cuve noire
      lcd.print("Choisir lambda") ;
      lcd.setCursor(0,1) ;
      lcd.print("= 550nm?") ;
      switchOnLed() ;
      delay(3990) ;
      sensor.takeMeasurements() ;
      lcd.clear() ;
      
      if (sensor.getCalibratedViolet() != 0) {
        lcd.clear() ;
        lcd.print("Reglage : 550nm") ; 
        delay(4000) ;
        modeVert = true ;  
        return ;
      }
      switchOffLed() ;          
    }
    previousMillis = millis() ;

    while (millis() - previousMillis < 4000) {      //Pendant quatre secondes on attend que l'on retire la cuve noire
      lcd.print("Choisir lambda") ;
      lcd.setCursor(0,1) ;
      lcd.print("= 570nm?") ;
      switchOnLed() ;
      delay(3990) ;
      sensor.takeMeasurements() ;
      lcd.clear() ;
      
      if (sensor.getCalibratedViolet() != 0) {
        lcd.clear() ;
        lcd.print("Reglage : 570nm") ; 
        delay(4000) ;
        modeJaune = true ;  
        return ;
      }
      switchOffLed() ;        
    }
    previousMillis = millis() ;

    while (millis() - previousMillis < 4000) {      //Pendant quatre secondes on attend que l'on retire la cuve noire
      lcd.print("Choisir lambda") ;
      lcd.setCursor(0,1) ;
      lcd.print("= 600nm?") ;
      switchOnLed() ;
      delay(3990) ;
      sensor.takeMeasurements() ;
      lcd.clear() ;
      
      if (sensor.getCalibratedViolet() != 0) {
        lcd.clear() ;
        lcd.print("Reglage : 600nm") ; 
        delay(4000) ;
        modeOrange = true ; 
        return ; 
      }
      switchOffLed() ;         
    }
    previousMillis = millis() ;

    while (millis() - previousMillis < 4000) {      //Pendant quatre secondes on attend que l'on retire la cuve noire
      lcd.print("Choisir lambda") ;
      lcd.setCursor(0,1) ;
      lcd.print("= 650nm?") ;
      switchOnLed() ;
      delay(3990) ;
      sensor.takeMeasurements() ;
      lcd.clear() ;
      
      if (sensor.getCalibratedViolet() != 0) {
        lcd.clear() ;
        lcd.print("Reglage : 650nm") ; 
        delay(4000) ;
        modeRouge = true ;  
        return ; 
      }
      switchOffLed() ;      
    }

  if (modeViolet == false && modeBleu == false && modeVert == false && modeJaune == false && modeOrange == false && modeRouge == false) { //Si aucune des longueurs d'ondes n'a été choisie on repasse au défilement normal des mesures
    lcd.clear() ;
    lcd.print("Retour au mode") ;
    lcd.setCursor(0,1) ;
    lcd.print("normal !") ;
    delay(4000) ;
    lcd.clear() ;
  }
}

void ledCalibration() {                     //Fonction qui permet de calibrer la LED au démarrage de l'appareil et donc de s'affranchir des différence qui peuvent exister entre les composants
  int violet = 0 ;
  ledIntensity = 0 ;                        //Cette valeur est proportionnellement liée au flux lumineux émis par la diode
  Serial.println("Réglage de la LED") ;
  while (violet < intensiteSeuil) {                   //On cherche à atteindre une certaine intensité lumineuse lue par le capteur sur le canal Violet, ici 5000. Tant que cette valeur n'est pas atteinte au augmente ledIntensity
    analogWrite(ledPin,ledIntensity) ; 
    delay(10) ;
    sensor.takeMeasurements() ;
    violet = sensor.getCalibratedViolet() ;
    Serial.print("Violet = ") ;
    Serial.println(violet) ;
    ledIntensity ++ ;                                 //On incrémente l'intensité perçue par la LED
  }
  Serial.print("Intensité de la LED = ") ;
  Serial.println(ledIntensity) ;
}
