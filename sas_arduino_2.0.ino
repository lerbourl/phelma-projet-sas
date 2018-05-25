#include <Wire.h>
#include <LiquidCrystal.h> // Inclusion de la librairie pour afficheur LCD 
#include <Keypad.h> // inclusion de la librairie pour clavier matriciel 
#include <Servo.h> // Inclusion de la librairie pour le servomoteur

#define SLAVE_ADDRESS 0x12

// --- initialisation d variables ---
int dataReceived = 0;
int dataToSend = 0xb0; // valeur par défaut
int demande_code = 0;
int compteur_reception = 0; // permet de gerer la réception d'une suit d'octets
int send_ack = 0; // envoi de l'acquittement, prévient que les données sont prêtes à être envoyées

// --- Déclaration des constantes pour clavier + écran ---

// --- Constantes utilisées avec le clavier 4x4 ---
const byte LIGNES = 4; // 4 lignes
const byte COLONNES = 4; //4 colonnes


// --- constantes des broches ---
const int C4 = 2; //declaration constante de broche
const int C3 = 3; //declaration constante de broche
const int C2 = 4; //declaration constante de broche
const int C1 = 5; //declaration constante de broche
const int RS = 8; //declaration constante de broche
const int E = 9; //declaration constante de broche
const int D4 = 10; //declaration constante de broche
const int D5 = 11; //declaration constante de broche
const int D6 = 12; //declaration constante de broche
const int D7 = 13; //declaration constante de broche
const int L4 = 14; //declaration constante de broche
const int L3 = 15; //declaration constante de broche
const int L2 = 16; //declaration constante de broche
const int L1 = 17; //declaration constante de broche

// --- Déclaration des variables globales pour clavier + écran ---

int code[4];
int k = 0; //compteur d'envoi du code

//--- Définition des touches
char touches[LIGNES][COLONNES] = 
{
  {'1', '2', '3', 'F'},
  {'4', '5', '6', 'E'},
  {'7', '8', '9', 'D'},
  {'A', '0', 'B', 'C'}
};

// tableaux de lignes et colonnes
byte BrochesLignes[LIGNES] = {L1, L2, L3, L4}; //connexions utilisées pour les broches de lignes du clavier
byte BrochesColonnes[COLONNES] = {C1, C2, C3, C4}; //connexions utilisées pour les broches de colonnes du clavier

char touche; // variable de stockage valeur touche appuyée


// --- Déclaration des objets utiles pour les fonctionnalités utilisées ---

LiquidCrystal lcd(RS, E, D4, D5, D6, D7);// Création d'un objet LiquidCrystal = initialisation LCD en mode 4 bits


// création d'un objet keypad = initialisation clavier
Keypad clavier = Keypad( makeKeymap(touches), BrochesLignes, BrochesColonnes, LIGNES, COLONNES);
// les broches de lignes sont automatiquement configurées en ENTREE avec pullup interne activé
// les broches de colonnes sont automatiquement configurées en SORTIE


// --- Déclaration constante de broche pour capteur ultrasons ---

const int pingPin = 6;
long cm;


// --- Déclaration variable globale pour moteur ---

int pos = 90; // Position initiale du moteur

// --- Déclaration objet utile moteur ---

Servo monServo;


// --- Fonctions de base ---

void setup() { // initialisation au démarrage de l'arduino
  Serial.begin(9600);
  monServo.attach(7); // Moteur branché sur la broche 7
  monServo.write(pos);
  lcd.begin(16, 2); // Initialise le LCD avec 16 colonnes x 2 lignes
  Wire.begin(SLAVE_ADDRESS); // lancement de l'i2c sur 0x12
  Wire.onReceive(receiveData); // définit la fonction pour la réception i2c
  Wire.onRequest(sendData); // définit la fonction d'envoi i2c
}

void loop() {
  if(demande_code){
    recup_code(); // cette fonction ne s'execute correctement que dans la loop!
    demande_code = 0;
  }
}



// --- fonctions à appeller lors de demandes de communication i2c ---

void receiveData(int byteCount) { //réception
  int etage = 0; // etage 0 ou 1 de l'ecran LCD
  String phrase = "";
  while (Wire.available()) {
    dataReceived = Wire.read();
    dataToSend = 0xb0; // vide le buffer au cas où une récuperation n'est pas finie
    Serial.print("Donnee recue : ");
    Serial.println(dataReceived); // debug
    // traitement de la réception des requêtes du raspberry
    if (compteur_reception == 0){
      if (dataReceived == 0xa1) {
        ouvrir_porte();
      }
      if (dataReceived == 0xa2) {
        fermer_porte();
      }
      if (dataReceived == 0xa3) {
        detecte_presence();
      }
      if (dataReceived == 0xa4) {
        compteur_reception = 1;
      }
      if (dataReceived == 0xa5) {
        aff_info_code();
      }
      if (dataReceived == 0xa6) {
        demande_code = 1;
      }
      if (dataReceived == 0xa7) {
        lcd.clear();
        Serial.println("ecran effacé");
      }
    }
    else{// en réception d'octets
      if(compteur_reception == 1){
        compteur_reception += dataReceived; //n de bits à recevoir + 1
      }
      else if(compteur_reception == 2){ // fin de réception
        etage = (dataReceived - 48); // conversion char/int, voir table ASCII
        lcd.setCursor(0, etage);
        lcd.print(phrase);
        Serial.print("etage ");
        Serial.println(etage);
        Serial.println(phrase);
        compteur_reception = 0;
      }
      else{ // lecture de la phrase
        phrase.concat(char(dataReceived));
        compteur_reception--;
      }
    }
  }
}

void sendData() { // envoi
  if (dataToSend == 0xb3){
    if(send_ack){
      Serial.println(0xb3);
      Wire.write(0xb3); // envoi de l'ack
      send_ack = 0;
    }
    else{
      Serial.println(cm); 
      Wire.write(cm); // puis envoi des données
      dataToSend = 0xb0;
    }
  }
  else if (dataToSend == 0xb1){
    if(send_ack){
      Serial.println(0xb1);
      Wire.write(0xb1); // envoi de l'ack
      send_ack = 0;
    }
    else{
      Serial.println(code[k]);
      Wire.write(code[k]); //envoi des 4 chiffres de code
      k++;
      if (k == 4){
        dataToSend = 0xb0;
      }
    }
  }
  else if (dataToSend == 0xb2){
    Serial.println(0xb2);
    Wire.write(0xb2); // envoi du code : annulation et sortie
    dataToSend = 0xb0;
  }
  else if (dataToSend == 0xb0){
    Serial.println(0xb0);
    Wire.write(0xb0); // état d'attente
  }
  else{
    Serial.println(dataToSend);
    Wire.write(dataToSend); // cas qui n'arrive jamais en théorie
  } 
}

// --- fonctions actionneurs et capteurs ---

void ouvrir_porte() { // 0ax1
  pos = 0;
  monServo.write(pos);
  lcd.clear();  //efface écran lcd
  Serial.print("Porte ouverte\n");
}

void fermer_porte() { // 0ax2
  pos = 90;
  monServo.write(pos);
  Serial.print("Porte fermée\n");
}

void detecte_presence() { // 0ax3
  long duration;
  pinMode(pingPin, OUTPUT); // Le pin est vu comme une sortie
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(pingPin, LOW);

  pinMode(pingPin, INPUT); // Le pin est ici vu comme une entrée
  duration = pulseIn(pingPin, HIGH); // Retourne la durée de l'impulsion dans duration

  // Conversion de la durée de l'impulsion en une distance
  cm = microsecondsToCentimeters(duration);
  Serial.print(cm);
  Serial.print("cm");
  Serial.println();
  send_ack = 1; // prêt à envoyer la distance
  dataToSend = 0xb3;
}

long microsecondsToCentimeters(long microseconds) {
  return microseconds / 29 / 2; // conversion durée/distance
}

void aff_info_code() { // 0xa5
  Serial.println("infos code");
  lcd.clear();
  lcd.print("A -> sortir");
  lcd.setCursor(0, 1);
  lcd.print("C -> corriger");
}

void recup_code() { // 0xa6
  touche = clavier.getKey(); // lecture de la touche appuyée
  int format_ok = 0;
  int i; // nombre de chiffres tappés
  while (!format_ok){
    i = 0;
    lcd.clear();
    lcd.print("B -> valider");
    lcd.setCursor(0, 1);
    lcd.print("Code:");
    lcd.setCursor(5, 1);
    
    touche = clavier.getKey(); // lecture de la touche appuyée
    while (touche != 'B' && touche != 'A'){
      if (touche != NO_KEY) {
        if (touche == 'C'){ // clear le code
          lcd.clear();
          lcd.print("B -> valider");
          lcd.setCursor(0, 1);
          lcd.print("Code:");
          lcd.setCursor(5, 1);
          i = 0;
          }
        else if(i < 4){ // 4 chiffres max!
          code[i] = (touche - '0'); //conversion en entier
          i++;
          lcd.print(touche);
        }
      }
      touche = clavier.getKey(); //récuperation de la touche
    }
    if (touche == 'A'){
      dataToSend = 0xb2;
      format_ok = 1; // on sort de la boucle et on envoi le code spécial
    }
    else if (i == 4){ // on boucle si on a pas les 4 chiffres de tappés
      dataToSend = 0xb1;
      k = 0;
      send_ack = 1; // prêt à envoyer le code
      format_ok = 1; // sortie
    }
  }
  lcd.clear();
}
