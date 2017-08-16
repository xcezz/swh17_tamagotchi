int data1 = 0;
int ok = -1;
 
int tag_yellow[14] = {2,48,56,48,48,48,65,56,57,55,51,70,56,3};
int tag_red[14] = {2,48,55,48,48,69,51,52,48,49,67,66,56,3};
int newtag[14] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0}; // used for read comparisons
 
void setup()
{
  Serial1.begin(9600);    // start serial to RFID reader
  Serial.begin(9600);  // start serial to PC
}
 
boolean comparetag(int aa[14], int bb[14])
{
  boolean ff = false;
  int fg = 0;
  for (int cc = 0 ; cc < 14 ; cc++)
  {
    if (aa[cc] == bb[cc])
    {
      fg++;
    }
  }
  if (fg == 14)
  {
    ff = true;
  }
  return ff;
}
 
void checkmytags() // compares each tag against the tag just read
{
  ok = 0; // this variable helps decision-making,
  // if it is 1 we have a match, zero is a read but no match,
  // -1 is no read attempt made
  if (comparetag(newtag, tag_red) == true)
  {
    ok++;
  }
}
 
void readTags()
{
  ok = -1;
 
  if (Serial1.available() > 0) 
  {
    // read tag numbers
    delay(100); // needed to allow time for the data to come in from the serial buffer.
 
    for (int z = 0 ; z < 14 ; z++) // read the rest of the tag
    {
      data1 = Serial1.read();
      newtag[z] = data1;
    }
    Serial1.flush(); // stops multiple reads
 
    // do the tags match up?
    checkmytags();
  }
 
  // now do something based on tag type
  if (ok > 0) // if we had a match
  {
    Serial.println("RED RFID ACCEPTED");
 
    ok = -1;
  }
  else if (ok == 0) // if we didn't have a match
  {
    Serial.println("REJECTED");
 
    ok = -1;
  }
}
 
void loop()
{
  readTags();
}
