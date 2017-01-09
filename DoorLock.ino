#include <Servo.h> 

Servo myservo;

void setup() 
{ 
  Serial.begin(9600);
} 

void loop() {
    if (Serial.available() > 0) {
      String input = Serial.readString();
        toggleDoor(input);
    }
}

void toggleDoor(String input){
  myservo.attach(9);

  if(input.equals("open")){
    myservo.write(135); // Unlock door
  }
  else {
    myservo.write(45);  // Lock door
  }
    delay(1000);
    myservo.detach();
    Serial.println("ok");
}

