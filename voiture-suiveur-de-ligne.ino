/**
 * @file voiture-suiveur-de-ligne.ino
 * @brief Code de commande orienté objet (C++) pour un robot suiveur de ligne.
 * 
 * Conçu spécifiquement pour le robot de la vidéo 'Hash Include Electronics'.
 * Ce code implémente des classes C++ pour une encapsulation et une modularité optimales.
 * La gestion des capteurs et celle des moteurs sont strictement isolées dans des classes dédiées.
 * Il intègre également la modification matérielle du registre TCCR0B à 7812.5 Hz.
 * 
 * @author Expert Programmation Embarquée C++
 * @date 2026-06-02
 */

#include <Arduino.h>

// =========================================================================
// 1. CONFIGURATION DU MATÉRIEL & PINOUT
// =========================================================================

// Vitesse cible PWM (0 - 255)
const int TARGET_SPEED = 180;

// =========================================================================
// 2. CLASSE C++ : GESTION DES CAPTEURS INFRAROUGES
// =========================================================================
class LineSensors {
public:
    /**
     * @struct State
     * @brief Encapsule l'état des deux capteurs IR.
     */
    struct State {
        bool right;
        bool left;
    };

    /**
     * @brief Constructeur configurant les broches matérielles.
     * @param pinRight Broche numérique du capteur droit.
     * @param pinLeft Broche numérique du capteur gauche.
     */
    LineSensors(uint8_t pinRight, uint8_t pinLeft) 
        : _pinRight(pinRight), _pinLeft(pinLeft) {}

    /**
     * @brief Initialise les broches physiques en mode entrée.
     */
    void begin() const {
        pinMode(_pinRight, INPUT);
        pinMode(_pinLeft, INPUT);
    }

    /**
     * @brief Lit et retourne l'état actuel des capteurs.
     */
    State read() const {
        return {
            .right = (digitalRead(_pinRight) == HIGH),
            .left = (digitalRead(_pinLeft) == HIGH)
        };
    }

private:
    const uint8_t _pinRight;
    const uint8_t _pinLeft;
};

// =========================================================================
// 3. CLASSE C++ : GESTION DU MOTOR DRIVER L298N
// =========================================================================
class MotorDriver {
public:
    /**
     * @brief Constructeur configurant l'ensemble des broches du driver L298N.
     */
    MotorDriver(uint8_t pinENA, uint8_t pinENB, 
                uint8_t pinIN1, uint8_t pinIN2, 
                uint8_t pinIN3, uint8_t pinIN4)
        : _pinENA(pinENA), _pinENB(pinENB),
          _pinIN1(pinIN1), _pinIN2(pinIN2),
          _pinIN3(pinIN3), _pinIN4(pinIN4) {}

    /**
     * @brief Initialise les broches physiques en mode sortie.
     */
    void begin() const {
        pinMode(_pinENA, OUTPUT);
        pinMode(_pinENB, OUTPUT);
        pinMode(_pinIN1, OUTPUT);
        pinMode(_pinIN2, OUTPUT);
        pinMode(_pinIN3, OUTPUT);
        pinMode(_pinIN4, OUTPUT);
        
        // Arrêt par défaut au démarrage
        stop();
    }

    /**
     * @brief Arrête immédiatement les deux moteurs.
     */
    void stop() const {
        rotate(0, 0);
    }

    /**
     * @brief Pilote les moteurs droit et gauche de façon asynchrone.
     * @param speedRight Consigne de vitesse moteur droit ([-255, 255]).
     * @param speedLeft Consigne de vitesse moteur gauche ([-255, 255]).
     */
    void rotate(int speedRight, int speedLeft) const {
        controlMotor(_pinENA, _pinIN1, _pinIN2, speedRight);
        controlMotor(_pinENB, _pinIN3, _pinIN4, speedLeft);
    }

private:
    const uint8_t _pinENA;
    const uint8_t _pinENB;
    const uint8_t _pinIN1;
    const uint8_t _pinIN2;
    const uint8_t _pinIN3;
    const uint8_t _pinIN4;

    /**
     * @brief Contrôle générique d'un canal de moteur unique.
     */
    void controlMotor(uint8_t pinEN, uint8_t pinINa, uint8_t pinINb, int speed) const {
        if (speed > 0) {
            digitalWrite(pinINa, HIGH);
            digitalWrite(pinINb, LOW);
            analogWrite(pinEN, speed);
        } else if (speed < 0) {
            digitalWrite(pinINa, LOW);
            digitalWrite(pinINb, HIGH);
            analogWrite(pinEN, -speed);
        } else {
            digitalWrite(pinINa, LOW);
            digitalWrite(pinINb, LOW);
            analogWrite(pinEN, 0);
        }
    }
};

// =========================================================================
// 4. INSTANCIATION DES OBJETS C++
// =========================================================================
// Capteurs IR : Broches D11 (Droit), D12 (Gauche)
LineSensors sensors(11, 12);

// Moteurs : ENA=5, ENB=6, IN1=7, IN2=8, IN3=9, IN4=10
MotorDriver motors(5, 6, 7, 8, 9, 10);

// =========================================================================
// 5. FONCTIONS STANDARD ARDUINO
// =========================================================================

void setup() {
    // Initialisation des modules C++
    sensors.begin();
    motors.begin();

    // --- OPTIMISATION DU TIMER 0 (Fréquence PWM) ---
    // Modification de la fréquence PWM des broches 5 (ENA) et 6 (ENB) à 7812.5 Hz.
    // Diviseur de fréquence (prescaler) configuré à 8.
    //
    // Note : delay(), millis() et micros() s'écouleront 8x plus vite.
    TCCR0B = (TCCR0B & 0b11111000) | 0b00000010;
}

void loop() {
    // 1. Module Capteurs : Lecture physique
    LineSensors::State sensorState = sensors.read();

    // 2. Logique de décision & Action Moteurs
    if (!sensorState.right && !sensorState.left) {
        // Aucun capteur sur la ligne noire (LOW/LOW) -> Avancer
        motors.rotate(TARGET_SPEED, TARGET_SPEED);
    } 
    else if (sensorState.right && !sensorState.left) {
        // Capteur droit sur la ligne (HIGH/LOW) -> Pivoter à droite
        // Moteur droit en arrière (-180), gauche en avant (180)
        motors.rotate(-TARGET_SPEED, TARGET_SPEED);
    } 
    else if (!sensorState.right && sensorState.left) {
        // Capteur gauche sur la ligne (LOW/HIGH) -> Pivoter à gauche
        // Moteur gauche en arrière (-180), droit en avant (180)
        motors.rotate(TARGET_SPEED, -TARGET_SPEED);
    } 
    else if (sensorState.right && sensorState.left) {
        // Les deux capteurs sur la ligne (HIGH/HIGH) -> Arrêt complet
        motors.stop();
    }
}
