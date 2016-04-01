
void preparePotValue(int pot, int val)
{
  switch(pot) {
    //update the corresponding digipot with the new sequence value
    case 0:
      isPot0Ready = 1;
      pot0Value = val;
    break;  
    case 1:
      isPot1Ready = 1;
      pot1Value = val;
    break;  
    case 2:
      isPot2Ready = 1;
      pot2Value = val;
    break;
    default:
      isPot3Ready = 1;
      pot3Value = val;
  } //switch
}

//check flags and update each pot if a new value is waiting to be written
void writeToPots()
{
  //it is necessary to move the i2c routines out of the callback. probably due to some interrupt handling!
  if(isPot0Ready) {
    DigipotWrite(0, pot0Value);
    isPot0Ready = 0;
  }
  if(isPot1Ready) {
    DigipotWrite(1, pot1Value);
    isPot1Ready = 0;
  }
  if(isPot2Ready) {
    DigipotWrite(2, pot2Value);
    isPot2Ready = 0;
  }
  if(isPot3Ready) {
    DigipotWrite(3, pot3Value);
    isPot3Ready = 0;
  }
}

//write a value on one of the four digipots in the IC
void DigipotWrite(byte pot, byte val)        
{
  i2c_send(digipot_addr, 0x40, 0xff);
  i2c_send(digipot_addr, 0xA0, 0xff);
  i2c_send(digipot_addr, addresses[pot], val);
}

//wrapper for I2C routines
void i2c_send(byte addr, byte a, byte b)      
{
  Wire.beginTransmission(addr);
  Wire.write(a);
  Wire.write(b);
  Wire.endTransmission();
}

