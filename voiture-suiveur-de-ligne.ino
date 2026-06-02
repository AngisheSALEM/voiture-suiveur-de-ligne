/**
 * @file voiture-suiveur-de-ligne.ino
 * @brief Code de commande modulaire pour un robot suiveur de ligne.
 * 
 * Conçu spécifiquement pour le robot de la vidéo 'Hash Include Electronics'.
 * Ce code sépare strictement la lecture des capteurs de l'action des moteurs (modulaire).
 * Il intègre également une optimisation matérielle pour modifier la fréquence PWM
 * du Timer 0 (broches 5 et 6) à 7812.5 Hz afin de permettre aux moteurs TT gear
 * de tourner de manière fluide à basse vitesse sans bloquer.
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

// --- Moteurs CC via L298N ---
// ENA et ENB doivent impérativement être connectés à des broches gérant le PWM.
// Les broches 5 (ENA) et 6 (ENB) sont reliées au Timer 0.
const int PIN_ENA = 5;  // Commande vitesse Moteur Droit (PWM)
const int PIN_ENB = 6;  // Commande vitesse Moteur Gauche (PWM)

// Signaux de direction du L298N
const int PIN_IN1 = 7;  // Moteur Droit Direction A
const int PIN_IN2 = 8;  // Moteur Droit Direction B
const int PIN_IN3 = 9;  // Moteur Gauche Direction A
const int PIN_IN4 = 10; // Moteur Gauche Direction B

// --- Vitesse de déplacement ---
const int TARGET_SPEED = 180; // Vitesse cible PWM (0 - 255)

// =========================================================================
// 2. STRUCTURES DE DONNÉES & MODULES
// =========================================================================

/**
 * @struct SensorState
 * @brief Structure propre stockant l'état des deux capteurs IR.
 * 
 * HIGH indique la détection de la ligne noire.
 * LOW indique la détection de la surface blanche.
 */
struct SensorState {
  bool right;
  bool left;
};

// =========================================================================
// 3. FONCTIONS MODULE CAPTEURS (Lecture)
// =========================================================================

/**
 * @brief Lit l'état actuel des capteurs infrarouges.
 * @return SensorState Structure contenant les états des capteurs droit et gauche.
 */
SensorState readSensors() {
  SensorState state;
  state.right = digitalRead(PIN_SENSOR_RIGHT);
  state.left = digitalRead(PIN_SENSOR_LEFT);
  return state;
}

// =========================================================================
// 4. FONCTIONS MODULE MOTEURS (Action)
// =========================================================================

/**
 * @brief Contrôle la vitesse et la direction des moteurs droit et gauche.
 * 
 * @param speedRight Vitesse du moteur droit (valeurs : [-255, 255]).
 *                   Positive = marche avant, Négative = marche arrière, 0 = arrêt.
 * @param speedLeft Vitesse du moteur gauche (valeurs : [-255, 255]).
 *                  Positive = marche avant, Négative = marche arrière, 0 = arrêt.
 */
void rotateMotor(int speedRight, int speedLeft) {
  
  // --- Gestion du Moteur Droit (Canal A) ---
  if (speedRight > 0) {
    // Marche avant
    digitalWrite(PIN_IN1, HIGH);
    digitalWrite(PIN_IN2, LOW);
    analogWrite(PIN_ENA, speedRight);
  } 
  else if (speedRight < 0) {
    // Marche arrière
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, HIGH);
    analogWrite(PIN_ENA, -speedRight); // analogWrite nécessite une valeur positive
  } 
  else {
    // Arrêt complet
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, LOW);
    analogWrite(PIN_ENA, 0);
  }

  // --- Gestion du Moteur Gauche (Canal B) ---
  if (speedLeft > 0) {
    // Marche avant
    digitalWrite(PIN_IN3, HIGH);
    digitalWrite(PIN_IN4, LOW);
    analogWrite(PIN_ENB, speedLeft);
  } 
  else if (speedLeft < 0) {
    // Marche arrière
    digitalWrite(PIN_IN3, LOW);
    digitalWrite(PIN_IN4, HIGH);
    analogWrite(PIN_ENB, -speedLeft); // analogWrite nécessite une valeur positive
  } 
  else {
    // Arrêt complet
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

  // Initialisation à l'arrêt pour la sécurité au démarrage
  rotateMotor(0, 0);

  // --- OPTIMISATION DU TIMER 0 ---
  // Par défaut, le Timer 0 a un diviseur (prescaler) de 64, générant du PWM à ~976.56 Hz.
  // En modifiant le prescaler à 8, on monte la fréquence à : 16 MHz / (8 * 256) = 7812.5 Hz.
  // Cela élimine le sifflement aigu des moteurs CC et résout les problèmes de blocage à basse vitesse.
  // Les broches affectées sont la D5 (ENA) et la D6 (ENB).
  //
  // NOTE D'EXPERT : Cette modification perturbe le rythme interne de l'Arduino.
  // Le temps s'écoulera 8 fois plus vite pour delay(), millis() et micros().
  // Exemple : delay(8000) durera en réalité 1 seconde standard (1000 ms).
  TCCR0B = (TCCR0B & 0b11111000) | 0b00000010;
}

// =========================================================================
// 6. BOUCLE PRINCIPALE (Logique de Décision)
// =========================================================================

void loop() {
  // --- Étape 1 : Lecture (Module Capteurs) ---
  // Nous récupérons l'état physique instantané du robot sans aucune action
  SensorState currentSensors = readSensors();

  // --- Étape 2 : Décision et Action (Module Moteurs) ---
  // Les entrées (capteurs) dictent de façon stricte les sorties (moteurs)
  
  if (currentSensors.right == LOW && currentSensors.left == LOW) {
    // Cas 1 : Aucun capteur ne détecte la ligne (sur le blanc) -> Avancer
    rotateMotor(TARGET_SPEED, TARGET_SPEED);
  } 
  else if (currentSensors.right == HIGH && currentSensors.left == LOW) {
    // Cas 2 : Capteur droit détecte la ligne -> Pivoter à droite
    // Moteur droit en arrière, Moteur gauche en avant
    rotateMotor(-TARGET_SPEED, TARGET_SPEED);
  } 
  else if (currentSensors.right == LOW && currentSensors.left == HIGH) {
    // Cas 3 : Capteur gauche détecte la ligne -> Pivoter à gauche
    // Moteur gauche en arrière, Moteur droit en avant
    rotateMotor(TARGET_SPEED, -TARGET_SPEED);
  } 
  else if (currentSensors.right == HIGH && currentSensors.left == HIGH) {
    // Cas 4 : Les deux détectent la ligne -> Arrêt complet
    rotateMotor(0, 0);
  }
}
