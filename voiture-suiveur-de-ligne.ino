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
 * --- NOUVELLES FONCTIONNALITÉS ---
 * 1. Capteur Ultrason HC-SR04 (Trig=D2, Echo=D3) : Détecte un obstacle en face à moins de 20 cm.
 * 2. Buzzer Passif (D4) : Siffle l'alarme d'iPhone (mélodie Radar / Classic Alarm) en cas d'obstacle, 
 *    et force l'arrêt complet de la voiture.
 * 
 * @author Expert Programmation Embarquée
 * @date 2026-06-02
 */

#include <Arduino.h>

// =========================================================================
// 1. CONFIGURATION DU MATÉRIEL & PINOUT
// =========================================================================

// --- Capteurs Infrarouges (IR) ---
const int PIN_SENSOR_RIGHT = 11; // Capteur Droit
const int PIN_SENSOR_LEFT = 12;  // Capteur Gauche

// --- Moteurs CC via L298N (Groupés par côté) ---
const int PIN_ENA = 5;  // Commande vitesse Moteurs Droits (PWM)
const int PIN_ENB = 6;  // Commande vitesse Moteurs Gauches (PWM)

// Signaux de direction du L298N
const int PIN_IN1 = 7;  // Moteurs Droits Direction A
const int PIN_IN2 = 8;  // Moteurs Droits Direction B
const int PIN_IN3 = 9;  // Moteurs Gauches Direction A
const int PIN_IN4 = 10; // Moteurs Gauches Direction B

// --- Capteur Ultrason HC-SR04 ---
const int PIN_TRIG = 2; // Broche Trigger
const int PIN_ECHO = 3; // Broche Echo

// --- Avertisseur Sonore ---
const int PIN_BUZZER = 4; // Broche Buzzer Passif

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
// 4. MODULE AVERTISSEUR (Buzzer & iPhone Alarm)
// =========================================================================

/**
 * @brief Joue le motif de la célèbre alarme "Radar / Classic Alarm" d'iOS.
 * 
 * NOTE D'EXPERT : L'augmentation de la fréquence du Timer 0 (pins 5 & 6)
 * fait que delay() s'exécute 8 fois plus vite. Nous appliquons donc un 
 * coefficient multiplicateur standard (* 8) pour conserver le rythme réel.
 */
void playIphoneAlarm() {
  // Mélodie du signal d'alerte iPhone "Radar"
  // Motif de 4 notes rapides ascendantes suivi d'une note longue
  int alarmMelody[] = {880, 988, 1047, 1318}; // Fréquences (La5, Si5, Do6, Mi6)
  int noteDurations[] = {100, 100, 100, 300}; // Durée de chaque note en ms

  for (int repeat = 0; repeat < 2; repeat++) { // Répète l'alarme 2 fois
    for (int i = 0; i < 4; i++) {
      // Émission du son sur le buzzer passif
      tone(PIN_BUZZER, alarmMelody[i], noteDurations[i] * 8);
      
      // Attente pour séparer proprement les notes
      delay((noteDurations[i] + 30) * 8);
    }
    // Courte pause entre les motifs répétitifs
    noTone(PIN_BUZZER);
    delay(200 * 8);
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

  // --- OPTIMISATION DU TIMER 0 (Fréquence PWM) ---
  // Modification de la fréquence PWM des broches 5 (ENA) et 6 (ENB) à 7812.5 Hz.
  // Diviseur de fréquence (prescaler) configuré à 8.
  //
  // Note importante : delay() et millis() s'écouleront 8 fois plus vite.
  // Exemple : delay(8000) prendra en réalité 1 seconde réelle.
  TCCR0B = (TCCR0B & 0b11111000) | 0b00000010;
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
