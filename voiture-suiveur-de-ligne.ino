/**
 * @file voiture-suiveur-de-ligne.ino
 * @brief Code de commande procédural (C++) pour un robot suiveur de ligne à 4 moteurs.
 * 
 * Conçu spécifiquement pour le robot de la vidéo 'Hash Include Electronics'.
 * Ce code est simple, modulaire et sans classes C++. Il sépare proprement la 
 * lecture des capteurs de l'actionnement des moteurs.
 * 
 * --- NOTE D'EXPERT SUR LA CONFIGURATION À 4 MOTEURS ---
 * Bien que la voiture possède 4 moteurs (4 roues motrices), le driver L298N ne possède 
 * que 2 canaux de sortie (A et B). Les moteurs sont donc connectés en parallèle :
 * 
 * 1. CÔTÉ DROIT : Les 2 moteurs droits (avant-droit et arrière-droit) sont branchés 
 *    ensemble en parallèle sur les sorties OUT1 & OUT2 du L298N.
 *    Ils sont pilotés simultanément par le groupe ENA, IN1, IN2.
 * 
 * 2. CÔTÉ GAUCHE : Les 2 moteurs gauches (avant-gauche et arrière-gauche) sont branchés 
 *    ensemble en parallèle sur les sorties OUT3 & OUT4 du L298N.
 *    Ils sont pilotés simultanément par le groupe ENB, IN3, IN4.
 * 
 * Ainsi, d'un point de vue logiciel, le code pilote 2 groupes de moteurs (Droit et Gauche),
 * ce qui rend ce programme 100% compatible et optimisé pour votre robot 4 roues motrices !
 * 
 * Note de fonctionnement : La voiture évite les espaces blancs, reste sur la ligne 
 * noire et ne recule jamais (elle s'arrête ou tourne en bloquant le côté intérieur).
 * 
 * @author Expert Programmation Embarquée
 * @date 2026-06-02
 */

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

// --- Vitesse de déplacement ---
const int TARGET_SPEED = 180; // Vitesse cible PWM (0 - 255)

// =========================================================================
// 2. VARIABLES GLOBALES DE STOCKAGE (Coordination)
// =========================================================================
// Ces variables permettent de faire la transition entre la lecture et l'action
int sensorRightState = LOW; // État du capteur droit (HIGH = noir, LOW = blanc)
int sensorLeftState = LOW;  // État du capteur gauche (HIGH = noir, LOW = blanc)

// =========================================================================
// 3. MODULE CAPTEURS (Lecture)
// =========================================================================

/**
 * @brief Lit l'état des capteurs IR et met à jour les variables globales.
 * 
 * Cette fonction s'occupe exclusivement de l'acquisition des données physiques.
 */
void readSensors() {
  sensorRightState = digitalRead(PIN_SENSOR_RIGHT);
  sensorLeftState = digitalRead(PIN_SENSOR_LEFT);
}

// =========================================================================
// 4. MODULE MOTEURS (Action)
// =========================================================================

/**
 * @brief Contrôle la vitesse des groupes de moteurs droit et gauche (sans marche arrière).
 * 
 * Comme le robot ne doit jamais reculer, les vitesses négatives ne sont pas gérées.
 * 
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
// 5. INITIALISATION DU SYSTÈME (Setup)
// =========================================================================

void setup() {
  // Configuration des broches des capteurs IR en entrée numérique
  pinMode(PIN_SENSOR_RIGHT, INPUT);
  pinMode(PIN_SENSOR_LEFT, INPUT);

  // Configuration des broches du L298N en sortie numérique
  pinMode(PIN_ENA, OUTPUT);
  pinMode(PIN_ENB, OUTPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);

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
// 6. BOUCLE PRINCIPALE (Logique de Décision)
// =========================================================================

void loop() {
  // --- Étape 1 : Lecture des capteurs ---
  readSensors();

  // --- Étape 2 : Décision de mouvement (sans recul) ---
  
  if (sensorRightState == LOW && sensorLeftState == LOW) {
    // Aucun capteur ne détecte la ligne (sur le blanc) -> Aller tout droit
    rotateMotor(TARGET_SPEED, TARGET_SPEED);
  } 
  else if (sensorRightState == HIGH && sensorLeftState == LOW) {
    // Le capteur droit détecte la ligne noire -> Pivoter à droite
    // On arrête les moteurs droits et on avance les moteurs gauches
    rotateMotor(0, TARGET_SPEED);
  } 
  else if (sensorRightState == LOW && sensorLeftState == HIGH) {
    // Le capteur gauche détecte la ligne noire -> Pivoter à gauche
    // On arrête les moteurs gauches et on avance les moteurs droits
    rotateMotor(TARGET_SPEED, 0);
  } 
  else if (sensorRightState == HIGH && sensorLeftState == HIGH) {
    // Les deux capteurs sont sur la ligne noire -> Arrêt complet
    rotateMotor(0, 0);
  }
}
