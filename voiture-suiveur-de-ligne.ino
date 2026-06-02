/**
 * @file voiture-suiveur-de-ligne.ino
 * @brief Code de commande procédural (C++) pour un robot suiveur de ligne à 4 moteurs.
 * 
 * Conçu spécifiquement pour le robot de la vidéo 'Hash Include Electronics'.
 * Ce code est simple, modulaire et sans classes C++. Il sépare proprement la 
 * lecture des capteurs de l'actionnement des moteurs.
 * 
 * --- NOTE SUR LES 4 MOTEURS ---
 * Les moteurs droits (avant et arrière) sont branchés en parallèle sur le canal A du L298N (OUT1 & OUT2).
 * Les moteurs gauches (avant et arrière) sont branchés en parallèle sur le canal B du L298N (OUT3 & OUT4).
 * 
 * --- NOUVEAU PINOUT & MATÉRIEL ---
 * - Moteurs Droits : ENA=11, IN1=10, IN2=9
 * - Moteurs Gauches : ENB=3, IN3=6, IN4=5
 * - Capteurs IR : Droite = 13 (Capteur 1), Gauche = 4 (Capteur 2)
 * - Capteur Ultrason : Echo=8, Trig=7
 * - Buzzer Passif : D2
 * 
 * --- NOTE D'EXPERT SUR LA FRÉQUENCE PWM & TIMER 2 ---
 * ENA et ENB étant désormais connectés aux broches 11 et 3, le contrôle du PWM 
 * dépend du **Timer 2** (et non plus du Timer 0).
 * Nous configurons donc le Timer 2 en mode Fast PWM avec un diviseur (prescaler) de 8.
 * Cela génère une fréquence parfaite de **7812.5 Hz** sur les sorties moteurs sans aucun sifflement !
 * 
 * **Avantage majeur :** Le Timer 0 restant inchangé, les fonctions temporelles standards 
 * de l'Arduino (delay(), millis()) fonctionnent désormais à vitesse normale et exacte !
 * (Pas besoin de multiplier par 8 les délais).
 * 
 * @author Expert Programmation Embarquée
 * @date 2026-06-02
 */

#include <Arduino.h>

// =========================================================================
// 1. CONFIGURATION DU MATÉRIEL & PINOUT (Mis à jour)
// =========================================================================

// --- Capteurs Infrarouges (IR) ---
const int PIN_SENSOR_RIGHT = 13; // Capteur Droit (Capteur 1)
const int PIN_SENSOR_LEFT = 4;   // Capteur Gauche (Capteur 2)

// --- Moteurs CC via L298N (Groupés par côté) ---
const int PIN_ENA = 11; // Commande vitesse Moteurs Droits (PWM sur Timer 2)
const int PIN_ENB = 3;  // Commande vitesse Moteurs Gauches (PWM sur Timer 2)

// Signaux de direction du L298N
const int PIN_IN1 = 10; // Moteurs Droits Direction A
const int PIN_IN2 = 9;  // Moteurs Droits Direction B
const int PIN_IN3 = 6;  // Moteurs Gauches Direction A
const int PIN_IN4 = 5;  // Moteurs Gauches Direction B

// --- Capteur Ultrason HC-SR04 ---
const int PIN_TRIG = 7; // Broche Trigger (Déclenchement)
const int PIN_ECHO = 8; // Broche Echo (Réception)

// --- Avertisseur Sonore ---
const int PIN_BUZZER = 2; // Broche Buzzer Passif

// --- Paramètres de comportement ---
const int TARGET_SPEED = 180;              // Vitesse cible PWM (0 - 255)
const int OBSTACLE_DISTANCE_THRESHOLD = 20; // Distance d'arrêt d'urgence (en cm)

// =========================================================================
// 2. VARIABLES GLOBALES DE STOCKAGE (Coordination)
// =========================================================================
int sensorRightState = LOW; // État du capteur droit (HIGH = noir, LOW = blanc)
int sensorLeftState = LOW;  // État du capteur gauche (HIGH = noir, LOW = blanc)

// =========================================================================
// 3. MODULE CAPTEURS (Lecture)
// =========================================================================

/**
 * @brief Lit l'état des capteurs IR et met à jour les variables globales.
 */
void readSensors() {
  sensorRightState = digitalRead(PIN_SENSOR_RIGHT);
  sensorLeftState = digitalRead(PIN_SENSOR_LEFT);
}

/**
 * @brief Calcule la distance de l'obstacle en face à l'aide de l'ultrason HC-SR04.
 * @return Distance en centimètres (cm). Renvoie 999 en cas d'erreur ou d'absence d'obstacle.
 */
long getDistance() {
  // Assure un signal propre
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);

  // Envoie une impulsion de 10 microsecondes
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // Lit la durée du signal de retour sur Echo (en microsecondes)
  // Timeout réglé à 30 ms (correspondant à ~5 mètres maximum d'analyse)
  long duration = pulseIn(PIN_ECHO, HIGH, 30000);

  // Si le signal a expiré sans retour
  if (duration == 0) {
    return 999;
  }

  // Calcul de la distance en cm : Vitesse du son (340 m/s) / 2
  long distance = duration * 0.0343 / 2;
  return distance;
}

// =========================================================================
// 4. MODULE AVERTISSEUR (Buzzer bare-metal indépendant des timers)
// =========================================================================

/**
 * @brief Génère un signal sonore précis sur le buzzer par commutation logicielle.
 * 
 * Cette méthode évite d'utiliser la fonction standard tone() qui entrerait en 
 * conflit avec la modification du registre du Timer 2 (broches 11 et 3).
 * 
 * @param frequency Fréquence du son en Hertz (Hz).
 * @param durationMs Durée du son en millisecondes (ms).
 */
void buzzerTone(unsigned int frequency, unsigned long durationMs) {
  // Période totale du signal en microsecondes
  unsigned long periodUs = 1000000UL / frequency;
  // Demi-période pour l'alternance haut/bas du signal carré
  unsigned long halfPeriodUs = periodUs / 2;
  
  unsigned long elapsedUs = 0;
  unsigned long durationUs = durationMs * 1000UL;
  
  while (elapsedUs < durationUs) {
    digitalWrite(PIN_BUZZER, HIGH);
    delayMicroseconds(halfPeriodUs);
    digitalWrite(PIN_BUZZER, LOW);
    delayMicroseconds(halfPeriodUs);
    elapsedUs += periodUs;
  }
}

/**
 * @brief Joue le motif de la célèbre alarme "Radar / Classic Alarm" d'iOS.
 * 
 * Les durées de temps utilisées ici sont réelles et précises (100% standards)
 * car le Timer 0 n'est pas altéré.
 */
void playIphoneAlarm() {
  // Mélodie du signal d'alerte iPhone "Radar"
  int alarmMelody[] = {880, 988, 1047, 1318}; // Fréquences (La5, Si5, Do6, Mi6)
  int noteDurations[] = {100, 100, 100, 300}; // Durée de chaque note en ms

  for (int repeat = 0; repeat < 2; repeat++) { // Répète l'alarme 2 fois
    for (int i = 0; i < 4; i++) {
      // Émission du son
      buzzerTone(alarmMelody[i], noteDurations[i]);
      
      // Attente standard pour séparer proprement les notes
      delay(noteDurations[i] + 30);
    }
    // Pause entre les motifs répétitifs
    delay(200);
  }
}

// =========================================================================
// 5. MODULE MOTEURS (Action)
// =========================================================================

/**
 * @brief Contrôle la vitesse des groupes de moteurs droit et gauche (sans marche arrière).
 * @param speedRight Vitesse des moteurs droits (0 à 255).
 * @param speedLeft Vitesse des moteurs gauches (0 à 255).
 */
void rotateMotor(int speedRight, int speedLeft) {
  
  // --- Contrôle des Moteurs Droits (OUT1 & OUT2) ---
  if (speedRight > 0) {
    digitalWrite(PIN_IN1, HIGH);
    digitalWrite(PIN_IN2, LOW);
    analogWrite(PIN_ENA, speedRight);
  } else {
    // Moteurs droits à l'arrêt
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, LOW);
    analogWrite(PIN_ENA, 0);
  }

  // --- Contrôle des Moteurs Gauches (OUT3 & OUT4) ---
  if (speedLeft > 0) {
    digitalWrite(PIN_IN3, HIGH);
    digitalWrite(PIN_IN4, LOW);
    analogWrite(PIN_ENB, speedLeft);
  } else {
    // Moteurs gauches à l'arrêt
    digitalWrite(PIN_IN3, LOW);
    digitalWrite(PIN_IN4, LOW);
    analogWrite(PIN_ENB, 0);
  }
}

// =========================================================================
// 6. INITIALISATION DU SYSTÈME (Setup)
// =========================================================================

void setup() {
  // Configuration des broches des capteurs IR en entrée
  pinMode(PIN_SENSOR_RIGHT, INPUT);
  pinMode(PIN_SENSOR_LEFT, INPUT);

  // Configuration des broches du L298N en sortie
  pinMode(PIN_ENA, OUTPUT);
  pinMode(PIN_ENB, OUTPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);

  // Configuration du capteur ultrason HC-SR04
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  // Configuration du Buzzer
  pinMode(PIN_BUZZER, OUTPUT);

  // Sécurité : Arrêt complet au démarrage
  rotateMotor(0, 0);

  // --- OPTIMISATION MATÉRIELLE DU TIMER 2 (Pins ENA=11 & ENB=3) ---
  // 1. Configurer le Timer 2 en mode Fast PWM 8-bit (bits WGM21 & WGM20 à 1 dans TCCR2A)
  TCCR2A = (TCCR2A & 0b11111100) | 0b00000011;

  // 2. Configurer le diviseur de fréquence (prescaler) à 8 (bits CS22=0, CS21=1, CS20=0 dans TCCR2B)
  // Fréquence résultante : 16 MHz / (8 * 256) = 7812.5 Hz.
  // Vos moteurs DC 4RM fonctionneront avec fluidité absolue, sans aucun sifflement !
  TCCR2B = (TCCR2B & 0b11111000) | 0b00000010;
}

// =========================================================================
// 7. BOUCLE PRINCIPALE (Logique de Décision)
// =========================================================================

void loop() {
  // --- Étape 1 : Vérification d'obstacle (Sécurité Prioritaire) ---
  long distance = getDistance();

  if (distance > 0 && distance <= OBSTACLE_DISTANCE_THRESHOLD) {
    // Obstacle détecté ! Arrêt d'urgence immédiat de la voiture
    rotateMotor(0, 0);

    // Déclenchement de l'avertisseur sonore (alarme iPhone)
    playIphoneAlarm();

    // On sort de loop() pour réévaluer la distance au prochain cycle sans exécuter le suivi de ligne
    return;
  }

  // --- Étape 2 : Suivi de ligne classique ---
  readSensors();

  if (sensorRightState == LOW && sensorLeftState == LOW) {
    // Aucun capteur ne détecte la ligne (sur le blanc) -> Aller tout droit
    rotateMotor(TARGET_SPEED, TARGET_SPEED);
  } 
  else if (sensorRightState == HIGH && sensorLeftState == LOW) {
    // Le capteur droit détecte la ligne noire -> Pivoter à droite
    // On arrête les moteurs droits (côté intérieur) et on avance les moteurs gauches (côté extérieur)
    rotateMotor(0, TARGET_SPEED);
  } 
  else if (sensorRightState == LOW && sensorLeftState == HIGH) {
    // Le capteur gauche détecte la ligne noire -> Pivoter à gauche
    // On arrête les moteurs gauches (côté intérieur) et on avance les moteurs droits (côté extérieur)
    rotateMotor(TARGET_SPEED, 0);
  } 
  else if (sensorRightState == HIGH && sensorLeftState == HIGH) {
    // Les deux capteurs sont sur la ligne noire -> Arrêt complet
    rotateMotor(0, 0);
  }
}
