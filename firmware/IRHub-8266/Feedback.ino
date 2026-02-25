/************ FEEDBACK ************/
void feedback(int ops) {

  switch (ops) {
    case 0:
      {
        MQTTsendInfoSatus();
        MQTTsendInfoBuild();
        MQTTsendInfoNetwork();
        MQTTsendInfoMQTT();
        break;
      }
    case 1:
      {
        MQTTsendUptime();
        break;
      }
    case 2:
      {

        MQTTsendInfoIR();

        break;
      }
    case 3:
      {
        MQTTsendAHT10();
        break;
      }
    case 4:
      {
        MQTTsendOutputs();
        break;
      }
    default:
      break;
  }
}
