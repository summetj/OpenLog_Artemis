
//Query each enabled sensor for its most recent data
void getData()
{
  measurementCount++;

  char tempData[50];
  outputData[0] = '\0'; //Clear string contents

  if (settings.logRTC)
  {
    //Decide if we are using the internal RTC or GPS for timestamps
    if (settings.getRTCfromGPS == false)
    {
      myRTC.getTime();

      if (settings.logDate)
      {
        char rtcDate[12]; //10/12/2019,
        if (settings.americanDateStyle == true)
          sprintf(rtcDate, "%02d/%02d/20%02d,", myRTC.month, myRTC.dayOfMonth, myRTC.year);
        else
          sprintf(rtcDate, "%02d/%02d/20%02d,", myRTC.dayOfMonth, myRTC.month, myRTC.year);
        strcat(outputData, rtcDate);
      }

      if (settings.logTime)
      {
        char rtcTime[13]; //09:14:37.41,
        int adjustedHour = myRTC.hour;
        if (settings.hour24Style == false)
        {
          if (adjustedHour > 12) adjustedHour -= 12;
        }
        sprintf(rtcTime, "%02d:%02d:%02d.%02d,", adjustedHour, myRTC.minute, myRTC.seconds, myRTC.hundredths);
        strcat(outputData, rtcTime);
      }
    } //end if use RTC for timestamp
    else //Use GPS for timestamp
    {
      Serial.println("Print GPS Timestamp / not yet implemented");
    }
  }

  if (settings.logA11)
  {
    unsigned int analog11 = analogRead(11);

    if (settings.logAnalogVoltages == true)
    {
      float voltage = analog11 * 2 / 16384.0;
      sprintf(tempData, "%.2f", voltage);
    }
    else
      sprintf(tempData, "%d,", analog11);

    strcat(outputData, tempData);
  }

  if (settings.logA12)
  {
    unsigned int analog12 = analogRead(12);

    if (settings.logAnalogVoltages == true)
    {
      float voltage = analog12 * 2 / 16384.0;
      sprintf(tempData, "%.2f", voltage);
    }
    else
      sprintf(tempData, "%d,", analog12);

    strcat(outputData, tempData);
  }

  if (settings.logA13)
  {
    unsigned int analog13 = analogRead(13);

    if (settings.logAnalogVoltages == true)
    {
      float voltage = analog13 * 2 / 16384.0;
      sprintf(tempData, "%.2f", voltage);
    }
    else
      sprintf(tempData, "%d,", analog13);

    strcat(outputData, tempData);
  }

  if (settings.logA32)
  {
    unsigned int analog32 = analogRead(32);

    if (settings.logAnalogVoltages == true)
    {
      float voltage = analog32 * 2 / 16384.0;
      sprintf(tempData, "%.2f,", voltage);
    }
    else
      sprintf(tempData, "%d,", analog32);

    strcat(outputData, tempData);
  }

  if (online.IMU)
  {
    if (myICM.dataReady())
    {
      myICM.getAGMT(); //Update values

      if (settings.logIMUAccel)
      {
        sprintf(tempData, "%.2f,%.2f,%.2f,", myICM.accX(), myICM.accY(), myICM.accZ());
        strcat(outputData, tempData);
      }
      if (settings.logIMUGyro)
      {
        sprintf(tempData, "%.2f,%.2f,%.2f,", myICM.gyrX(), myICM.gyrY(), myICM.gyrZ());
        strcat(outputData, tempData);
      }
      if (settings.logIMUMag)
      {
        sprintf(tempData, "%.2f,%.2f,%.2f,", myICM.magX(), myICM.magY(), myICM.magZ());
        strcat(outputData, tempData);
      }
      if (settings.logIMUTemp)
      {
        sprintf(tempData, "%.2f,", myICM.temp());
        strcat(outputData, tempData);
      }
    }
  }

  //Append all external sensor data on linked list to outputData
  gatherDeviceValues();

  if (settings.logHertz)
  {
    uint64_t currentMillis;

    //If we are sleeping between readings then we cannot rely on millis() as it is powered down
    //Used RTC instead
    if (settings.usBetweenReadings >= maxUsBeforeSleep)
    {
      currentMillis = rtcMillis();
    }
    else
    {
      //Calculate the actual update rate based on the sketch start time and the
      //number of updates we've completed.
      currentMillis = millis();
    }

    float actualRate = measurementCount * 1000.0 / (currentMillis - measurementStartTime);
    sprintf(tempData, "%.02f,", actualRate); //Hz
    strcat(outputData, tempData);
  }

  strcat(outputData, "\n");

  totalCharactersPrinted += strlen(outputData);
}

//Read values from the devices on the node list
//Append values to outputData
void gatherDeviceValues()
{
  char tempData[100];

  //Step through list, printing values as we go
  node *temp = head;
  while (temp != NULL)
  {
    //If this node successfully begin()'d
    if (temp->online == true)
    {
      //Switch on device type to set proper class and setting struct
      switch (temp->deviceType)
      {
        case DEVICE_MULTIPLEXER:
          {
            //No data to print for a mux
          }
          break;
        case DEVICE_LOADCELL_NAU7802:
          {
            NAU7802 *nodeDevice = (NAU7802 *)temp->classPtr;
            struct_NAU7802 *nodeSetting = (struct_NAU7802 *)temp->configPtr; //Create a local pointer that points to same spot as node does

            if (nodeSetting->log == true)
            {
              openConnection(temp->muxAddress, temp->portNumber); //Connect to this device through muxes as needed

              float currentWeight = nodeDevice->getWeight(false, nodeSetting->averageAmount); //Do not allow negative weights, take average of X readings
              sprintf(tempData, "%.*f,", nodeSetting->decimalPlaces, currentWeight);
              strcat(outputData, tempData);
            }
          }
          break;
        case DEVICE_DISTANCE_VL53L1X:
          {
            SFEVL53L1X *nodeDevice = (SFEVL53L1X *)temp->classPtr;
            struct_VL53L1X *nodeSetting = (struct_VL53L1X *)temp->configPtr;

            if (nodeSetting->log == true)
            {
              openConnection(temp->muxAddress, temp->portNumber);

              if (nodeSetting->logDistance)
              {
                sprintf(tempData, "%d,", nodeDevice->getDistance());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logRangeStatus)
              {
                sprintf(tempData, "%d,", nodeDevice->getRangeStatus());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logSignalRate)
              {
                sprintf(tempData, "%d,", nodeDevice->getSignalRate());
                strcat(outputData, tempData);
              }
            }
          }
          break;
        case DEVICE_GPS_UBLOX:
          {
            SFE_UBLOX_GPS *nodeDevice = (SFE_UBLOX_GPS *)temp->classPtr;
            struct_uBlox *nodeSetting = (struct_uBlox *)temp->configPtr;

            if (nodeSetting->log == true)
            {
              openConnection(temp->muxAddress, temp->portNumber);

              if (nodeSetting->logDate)
              {
                if (settings.americanDateStyle == true)
                  sprintf(tempData, "%02d/%02d/%d,", nodeDevice->getMonth(), nodeDevice->getDay(), nodeDevice->getYear());
                else
                  sprintf(tempData, "%02d/%02d/%d,", nodeDevice->getDay(), nodeDevice->getMonth(), nodeDevice->getYear());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logTime)
              {
                int adjustedHour = nodeDevice->getHour();
                if (settings.hour24Style == false)
                  if (adjustedHour > 12) adjustedHour -= 12;
                sprintf(tempData, "%02d:%02d:%02d.%03d,", adjustedHour, nodeDevice->getMinute(), nodeDevice->getSecond(), nodeDevice->getMillisecond());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logPosition)
              {
                sprintf(tempData, "%d,%d,", nodeDevice->getLatitude(), nodeDevice->getLongitude());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logAltitude)
              {
                sprintf(tempData, "%d,", nodeDevice->getAltitude());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logAltitudeMSL)
              {
                sprintf(tempData, "%d,", nodeDevice->getAltitudeMSL());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logSIV)
              {
                sprintf(tempData, "%d,", nodeDevice->getSIV());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logFixType)
              {
                sprintf(tempData, "%d,", nodeDevice->getFixType());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logCarrierSolution)
              {
                sprintf(tempData, "%d,", nodeDevice->getCarrierSolutionType());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logGroundSpeed)
              {
                sprintf(tempData, "%d,", nodeDevice->getGroundSpeed());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logHeadingOfMotion)
              {
                sprintf(tempData, "%d,", nodeDevice->getHeading());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logpDOP)
              {
                sprintf(tempData, "%d,", nodeDevice->getPDOP());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logiTOW)
              {
                sprintf(tempData, "%d,", nodeDevice->getTimeOfWeek());
                strcat(outputData, tempData);
              }
            }
          }
          break;
        case DEVICE_PROXIMITY_VCNL4040:
          {
            VCNL4040 *nodeDevice = (VCNL4040 *)temp->classPtr;
            struct_VCNL4040 *nodeSetting = (struct_VCNL4040 *)temp->configPtr;

            if (nodeSetting->log == true)
            {
              openConnection(temp->muxAddress, temp->portNumber);

              if (nodeSetting->logProximity)
              {
                sprintf(tempData, "%d,", nodeDevice->getProximity());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logAmbientLight)
              {
                sprintf(tempData, "%d,", nodeDevice->getAmbient());
                strcat(outputData, tempData);
              }
            }
          }
          break;
        case DEVICE_TEMPERATURE_TMP117:
          {
            TMP117 *nodeDevice = (TMP117 *)temp->classPtr;
            struct_TMP117 *nodeSetting = (struct_TMP117 *)temp->configPtr;

            if (nodeSetting->log == true)
            {
              openConnection(temp->muxAddress, temp->portNumber);

              if (nodeSetting->logTemperature)
              {
                sprintf(tempData, "%.04f,", nodeDevice->readTempC()); //Resolution to 0.0078°C, accuracy of 0.1°C
                strcat(outputData, tempData);
              }
            }
          }
          break;
        case DEVICE_PRESSURE_MS5637:
          {
            MS5637 *nodeDevice = (MS5637 *)temp->classPtr;
            struct_MS5637 *nodeSetting = (struct_MS5637 *)temp->configPtr;

            if (nodeSetting->log == true)
            {
              openConnection(temp->muxAddress, temp->portNumber);

              if (nodeSetting->logPressure)
              {
                sprintf(tempData, "%.02f,", nodeDevice->getPressure());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logTemperature)
              {
                sprintf(tempData, "%.02f,", nodeDevice->getTemperature());
                strcat(outputData, tempData);
              }
            }
          }
          break;
        case DEVICE_PRESSURE_LPS25HB:
          {
            LPS25HB *nodeDevice = (LPS25HB *)temp->classPtr;
            struct_LPS25HB *nodeSetting = (struct_LPS25HB *)temp->configPtr;
            if (nodeSetting->log == true)
            {
              openConnection(temp->muxAddress, temp->portNumber);

              if (nodeSetting->logPressure)
              {
                sprintf(tempData, "%.02f,", nodeDevice->getPressure_hPa());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logTemperature)
              {
                sprintf(tempData, "%.02f,", nodeDevice->getTemperature_degC());
                strcat(outputData, tempData);
              }
            }
          }
          break;
        case DEVICE_PHT_BME280:
          {
            BME280 *nodeDevice = (BME280 *)temp->classPtr;
            struct_BME280 *nodeSetting = (struct_BME280 *)temp->configPtr;
            if (nodeSetting->log == true)
            {
              openConnection(temp->muxAddress, temp->portNumber);

              if (nodeSetting->logPressure)
              {
                sprintf(tempData, "%.02f,", nodeDevice->readFloatPressure());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logHumidity)
              {
                sprintf(tempData, "%.02f,", nodeDevice->readFloatHumidity());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logAltitude)
              {
                sprintf(tempData, "%.02f,", nodeDevice->readFloatAltitudeMeters());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logTemperature)
              {
                sprintf(tempData, "%.02f,", nodeDevice->readTempC());
                strcat(outputData, tempData);
              }
            }
          }
          break;
        case DEVICE_UV_VEML6075:
          {
            VEML6075 *nodeDevice = (VEML6075 *)temp->classPtr;
            struct_VEML6075 *nodeSetting = (struct_VEML6075 *)temp->configPtr;
            if (nodeSetting->log == true)
            {
              openConnection(temp->muxAddress, temp->portNumber);

              if (nodeSetting->logUVA)
              {
                sprintf(tempData, "%.02f,", nodeDevice->uva());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logUVB)
              {
                sprintf(tempData, "%.02f,", nodeDevice->uvb());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logUVIndex)
              {
                sprintf(tempData, "%.02f,", nodeDevice->index());
                strcat(outputData, tempData);
              }
            }
          }
          break;

        case DEVICE_VOC_CCS811:
          {
            CCS811 *nodeDevice = (CCS811 *)temp->classPtr;
            struct_CCS811 *nodeSetting = (struct_CCS811 *)temp->configPtr;
            if (nodeSetting->log == true)
            {
              openConnection(temp->muxAddress, temp->portNumber);

              nodeDevice->readAlgorithmResults();
              if (nodeSetting->logTVOC)
              {
                sprintf(tempData, "%d,", nodeDevice->getTVOC());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logCO2)
              {
                sprintf(tempData, "%d,", nodeDevice->getCO2());
                strcat(outputData, tempData);
              }
            }
          }
          break;
        case DEVICE_VOC_SGP30:
          {
            SGP30 *nodeDevice = (SGP30 *)temp->classPtr;
            struct_SGP30 *nodeSetting = (struct_SGP30 *)temp->configPtr;
            if (nodeSetting->log == true)
            {
              openConnection(temp->muxAddress, temp->portNumber);

              nodeDevice->measureAirQuality();
              if (nodeSetting->logTVOC)
              {
                sprintf(tempData, "%d,", nodeDevice->TVOC);
                strcat(outputData, tempData);
              }
              if (nodeSetting->logCO2)
              {
                sprintf(tempData, "%d,", nodeDevice->CO2);
                strcat(outputData, tempData);
              }
            }
          }
          break;
        case DEVICE_CO2_SCD30:
          {
            SCD30 *nodeDevice = (SCD30 *)temp->classPtr;
            struct_SCD30 *nodeSetting = (struct_SCD30 *)temp->configPtr;
            if (nodeSetting->log == true)
            {
              openConnection(temp->muxAddress, temp->portNumber);

              if (nodeSetting->logCO2)
              {
                sprintf(tempData, "%d,", nodeDevice->getCO2());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logHumidity)
              {
                sprintf(tempData, "%.02f,", nodeDevice->getHumidity());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logTemperature)
              {
                sprintf(tempData, "%.02f,", nodeDevice->getTemperature());
                strcat(outputData, tempData);
              }
            }
          }
          break;
        case DEVICE_PHT_MS8607:
          {
            MS8607 *nodeDevice = (MS8607 *)temp->classPtr;
            struct_MS8607 *nodeSetting = (struct_MS8607 *)temp->configPtr;
            if (nodeSetting->log == true)
            {
              openConnection(temp->muxAddress, temp->portNumber);

              if (nodeSetting->logHumidity)
              {
                sprintf(tempData, "%.02f,", nodeDevice->getHumidity());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logPressure)
              {
                sprintf(tempData, "%.02f,", nodeDevice->getPressure());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logTemperature)
              {
                sprintf(tempData, "%.02f,", nodeDevice->getTemperature());
                strcat(outputData, tempData);
              }
            }
          }
          break;
        case DEVICE_TEMPERATURE_MCP9600:
          {
            MCP9600 *nodeDevice = (MCP9600 *)temp->classPtr;
            struct_MCP9600 *nodeSetting = (struct_MCP9600 *)temp->configPtr;
            if (nodeSetting->log == true)
            {
              openConnection(temp->muxAddress, temp->portNumber);

              if (nodeSetting->logTemperature)
              {
                sprintf(tempData, "%.02f,", nodeDevice->getThermocoupleTemp());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logAmbientTemperature)
              {
                sprintf(tempData, "%.02f,", nodeDevice->getAmbientTemp());
                strcat(outputData, tempData);
              }
            }
          }
          break;
        case DEVICE_HUMIDITY_AHT20:
          {
            AHT20 *nodeDevice = (AHT20 *)temp->classPtr;
            struct_AHT20 *nodeSetting = (struct_AHT20 *)temp->configPtr;
            if (nodeSetting->log == true)
            {
              openConnection(temp->muxAddress, temp->portNumber);

              if (nodeSetting->logHumidity)
              {
                sprintf(tempData, "%.02f,", nodeDevice->getHumidity());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logTemperature)
              {
                sprintf(tempData, "%.02f,", nodeDevice->getTemperature());
                strcat(outputData, tempData);
              }
            }
          }
          break;
        case DEVICE_HUMIDITY_SHTC3:
          {
            SHTC3 *nodeDevice = (SHTC3 *)temp->classPtr;
            struct_SHTC3 *nodeSetting = (struct_SHTC3 *)temp->configPtr;
            if (nodeSetting->log == true)
            {
              openConnection(temp->muxAddress, temp->portNumber);

              nodeDevice->update();
              if (nodeSetting->logHumidity)
              {
                sprintf(tempData, "%.02f,", nodeDevice->toPercent());
                strcat(outputData, tempData);
              }
              if (nodeSetting->logTemperature)
              {
                sprintf(tempData, "%.02f,", nodeDevice->toDegC());
                strcat(outputData, tempData);
              }
            }
          }
          break;
        default:
          Serial.printf("printDeviceValue unknown device type: %s\n", getDeviceName(temp->deviceType));
          break;
      }

    }
    temp = temp->next;
  }
}

//Step through the node list and print helper text for the enabled readings
void printHelperText()
{
  char helperText[1000];
  helperText[0] = '\0';

  if (settings.logRTC)
  {
    //Decide if we are using the internal RTC or GPS for timestamps
    if (settings.getRTCfromGPS == false)
    {
      if (settings.logDate)
        strcat(helperText, "rtcDate,");
      if (settings.logTime)
        strcat(helperText, "rtcTime,");
    }
  } //end if use RTC for timestamp
  else //Use GPS for timestamp
  {
  }

  if (settings.logA11)
    strcat(helperText, "analog_11,");

  if (settings.logA12)
    strcat(helperText, "analog_12,");

  if (settings.logA13)
    strcat(helperText, "analog_13,");

  if (settings.logA32)
    strcat(helperText, "analog_32,");

  if (online.IMU)
  {
    if (settings.logIMUAccel)
      strcat(helperText, "aX,aY,aZ,");
    if (settings.logIMUGyro)
      strcat(helperText, "gX,gY,gZ,");
    if (settings.logIMUMag)
      strcat(helperText, "mX,mY,mZ,");
    if (settings.logIMUTemp)
      strcat(helperText, "imu_degC,");
  }

  //Step through list, printing values as we go
  node *temp = head;
  while (temp != NULL)
  {

    //If this node successfully begin()'d
    if (temp->online == true)
    {
      //Switch on device type to set proper class and setting struct
      switch (temp->deviceType)
      {
        case DEVICE_MULTIPLEXER:
          {
            //No data to print for a mux
          }
          break;
        case DEVICE_LOADCELL_NAU7802:
          {
            struct_NAU7802 *nodeSetting = (struct_NAU7802 *)temp->configPtr;
            if (nodeSetting->log)
              strcat(helperText, "weight(no unit),");
          }
          break;
        case DEVICE_DISTANCE_VL53L1X:
          {
            struct_VL53L1X *nodeSetting = (struct_VL53L1X *)temp->configPtr;
            if (nodeSetting->log)
            {
              if (nodeSetting->logDistance)
                strcat(helperText, "distance_mm,");
              if (nodeSetting->logRangeStatus)
                strcat(helperText, "distance_rangeStatus(0=good),");
              if (nodeSetting->logSignalRate)
                strcat(helperText, "distance_signalRate,");
            }
          }
          break;
        case DEVICE_GPS_UBLOX:
          {
            struct_uBlox *nodeSetting = (struct_uBlox *)temp->configPtr;
            if (nodeSetting->log)
            {
              if (nodeSetting->logDate)
                strcat(helperText, "gps_Date,");
              if (nodeSetting->logTime)
                strcat(helperText, "gps_Time,");
              if (nodeSetting->logPosition)
                strcat(helperText, "gps_Lat,gps_Long,");
              if (nodeSetting->logAltitude)
                strcat(helperText, "gps_Alt,");
              if (nodeSetting->logAltitudeMSL)
                strcat(helperText, "gps_AltMSL,");
              if (nodeSetting->logSIV)
                strcat(helperText, "gps_SIV,");
              if (nodeSetting->logFixType)
                strcat(helperText, "gps_FixType,");
              if (nodeSetting->logCarrierSolution)
                strcat(helperText, "gps_CarrierSolution,");
              if (nodeSetting->logGroundSpeed)
                strcat(helperText, "gps_GroundSpeed,");
              if (nodeSetting->logHeadingOfMotion)
                strcat(helperText, "gps_Heading,");
              if (nodeSetting->logpDOP)
                strcat(helperText, "gps_pDOP,");
              if (nodeSetting->logiTOW)
                strcat(helperText, "gps_iTOW,");
            }
          }
          break;
        case DEVICE_PROXIMITY_VCNL4040:
          {
            struct_VCNL4040 *nodeSetting = (struct_VCNL4040 *)temp->configPtr;
            if (nodeSetting->log)
            {
              if (nodeSetting->logProximity)
                strcat(helperText, "prox(no unit),");
              if (nodeSetting->logAmbientLight)
                strcat(helperText, "ambient_lux,");
            }
          }
          break;
        case DEVICE_TEMPERATURE_TMP117:
          {
            struct_TMP117 *nodeSetting = (struct_TMP117 *)temp->configPtr;
            if (nodeSetting->log)
            {
              if (nodeSetting->logTemperature)
                strcat(helperText, "degC,");
            }
          }
          break;
        case DEVICE_PRESSURE_MS5637:
          {
            struct_MS5637 *nodeSetting = (struct_MS5637 *)temp->configPtr;
            if (nodeSetting->log)
            {
              if (nodeSetting->logPressure)
                strcat(helperText, "pressure_hPa,");
              if (nodeSetting->logTemperature)
                strcat(helperText, "pressure_degC,");
            }
          }
          break;
        case DEVICE_PRESSURE_LPS25HB:
          {
            struct_LPS25HB *nodeSetting = (struct_LPS25HB *)temp->configPtr;
            if (nodeSetting->log)
            {
              if (nodeSetting->logPressure)
                strcat(helperText, "pressure_hPa,");
              if (nodeSetting->logTemperature)
                strcat(helperText, "pressure_degC,");
            }
          }
          break;
        case DEVICE_PHT_BME280:
          {
            struct_BME280 *nodeSetting = (struct_BME280 *)temp->configPtr;
            if (nodeSetting->log)
            {
              if (nodeSetting->logPressure)
                strcat(helperText, "pressure_Pa,");
              if (nodeSetting->logHumidity)
                strcat(helperText, "humidity_%,");
              if (nodeSetting->logAltitude)
                strcat(helperText, "altitude_m,");
              if (nodeSetting->logTemperature)
                strcat(helperText, "temp_degC,");
            }
          }
          break;
        case DEVICE_UV_VEML6075:
          {
            struct_VEML6075 *nodeSetting = (struct_VEML6075 *)temp->configPtr;
            if (nodeSetting->log)
            {
              if (nodeSetting->logUVA)
                strcat(helperText, "uva,");
              if (nodeSetting->logUVB)
                strcat(helperText, "uvb,");
              if (nodeSetting->logUVIndex)
                strcat(helperText, "uvIndex,");
            }
          }
          break;
        case DEVICE_VOC_CCS811:
          {
            struct_CCS811 *nodeSetting = (struct_CCS811 *)temp->configPtr;
            if (nodeSetting->log)
            {
              if (nodeSetting->logTVOC)
                strcat(helperText, "tvoc_ppb,");
              if (nodeSetting->logCO2)
                strcat(helperText, "co2_ppm,");
            }
          }
          break;
        case DEVICE_VOC_SGP30:
          {
            struct_SGP30 *nodeSetting = (struct_SGP30 *)temp->configPtr;
            if (nodeSetting->log)
            {
              if (nodeSetting->logTVOC)
                strcat(helperText, "tvoc_ppb,");
              if (nodeSetting->logCO2)
                strcat(helperText, "co2_ppm,");
            }
          }
          break;
        case DEVICE_CO2_SCD30:
          {
            struct_SCD30 *nodeSetting = (struct_SCD30 *)temp->configPtr;
            if (nodeSetting->log)
            {
              if (nodeSetting->logCO2)
                strcat(helperText, "co2_ppm,");
              if (nodeSetting->logHumidity)
                strcat(helperText, "humidity_%,");
              if (nodeSetting->logTemperature)
                strcat(helperText, "degC,");
            }
          }
          break;
        case DEVICE_PHT_MS8607:
          {
            struct_MS8607 *nodeSetting = (struct_MS8607 *)temp->configPtr;
            if (nodeSetting->log)
            {
              if (nodeSetting->logHumidity)
                strcat(helperText, "humidity_%,");
              if (nodeSetting->logPressure)
                strcat(helperText, "hPa,");
              if (nodeSetting->logTemperature)
                strcat(helperText, "degC,");
            }
          }
          break;
        case DEVICE_TEMPERATURE_MCP9600:
          {
            struct_MCP9600 *nodeSetting = (struct_MCP9600 *)temp->configPtr;
            if (nodeSetting->log)
            {
              if (nodeSetting->logTemperature)
                strcat(helperText, "thermo_degC,");
              if (nodeSetting->logAmbientTemperature)
                strcat(helperText, "thermo_ambientDegC,");
            }
          }
          break;
        case DEVICE_HUMIDITY_AHT20:
          {
            struct_AHT20 *nodeSetting = (struct_AHT20 *)temp->configPtr;
            if (nodeSetting->log)
            {
              if (nodeSetting->logHumidity)
                strcat(helperText, "humidity_%,");
              if (nodeSetting->logTemperature)
                strcat(helperText, "degC,");
            }
          }
          break;
        case DEVICE_HUMIDITY_SHTC3:
          {
            struct_SHTC3 *nodeSetting = (struct_SHTC3 *)temp->configPtr;
            if (nodeSetting->log)
            {
              if (nodeSetting->logHumidity)
                strcat(helperText, "humidity_%,");
              if (nodeSetting->logTemperature)
                strcat(helperText, "degC,");
            }
          }
          break;
        default:
          Serial.printf("\nprinterHelperText device not found: %d\n", temp->deviceType);
          break;
      }
    }
    temp = temp->next;
  }

  if (settings.logHertz)
    strcat(helperText, "output_Hz,");

  strcat(helperText, "\n");

  Serial.print(helperText);
}
//If certain devices are attached, we need to reduce the I2C max speed
void setMaxI2CSpeed()
{
  uint32_t maxSpeed = 400000; //Assume 400kHz

  //Search nodes for MCP9600s and Ublox modules
  node *temp = head;
  while (temp != NULL)
  {
    if (temp->deviceType == DEVICE_TEMPERATURE_MCP9600)
    {
      //TODO Are we sure the MCP9600, begin'd() on the bus, but not logged will behave when bus is 400kHz?
      //Check if logging is enabled
      struct_MCP9600 *sensor = (struct_MCP9600*)temp->configPtr;
      if (sensor->log == true)
        maxSpeed = 100000;
    }

    if (temp->deviceType == DEVICE_GPS_UBLOX)
    {
      //Check if i2cSpeed is lowered
      struct_uBlox *sensor = (struct_uBlox*)temp->configPtr;
      if (sensor->i2cSpeed == 100000)
        maxSpeed = 100000;
    }

    temp = temp->next;
  }

  //If user wants to limit the I2C bus speed, do it here
  if (maxSpeed > settings.qwiicBusMaxSpeed)
    maxSpeed = settings.qwiicBusMaxSpeed;

  qwiic.setClock(maxSpeed);
}
