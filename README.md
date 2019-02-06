# Magic Cube - AWS Sumerian

This repo is a small end to end proof of concept on using AWS Sumerian with AWS IoT data

## Hardware

### BOM

***Note**: most of this stuff you can find extremely cheap off aliexpress if you shop around*

- 1x - [NodeMCU ESP8266](https://amzn.to/2BgZhnF) : $8.39
- 1x - [Adafruit Micro Lipo w/MicroUSB Jack](https://amzn.to/2Da2yp0) : $10.11
- 1x - [Lithium Ion Battery 1000mAh](https://www.sparkfun.com/products/13813) : $9.95
- 1x - [MMA8451 Accelerometer](https://amzn.to/2t5DV8d) : $9.44

### Schematic

![ESP8266 Schematic](aws-iot-esp8266/design/esp8266-circuit-design.png)

### Dependencies

We need to deploy the IoT Certificate vending machine lambda code somewhere accessible for CloudFormation.

```bash
## The command I use to deploy the IoT CVM code to my bucket
aws s3 cp aws-iot-cvm/iot-cvm.zip s3://devopstar/resources/aws-iot-vpn-spawner/iot-cvm.zip
```

This reference must be updated in the `aws-iot-cvm/iot-cvm-params.json` parameters file else it will default to the version in my bucket. This is only applicable if you'd prefer to deploy your own.

```json
{
    "ParameterKey": "LambdaCodeBucket",
    "ParameterValue": "devopstar" # Bucket Name
},
{
    "ParameterKey": "LambdaCodeS3Key",
    "ParameterValue": "resources/aws-iot-vpn-spawner/iot-cvm.zip" # Code Location
}
```

Deploys a IoT Vending machine instances that can be used to generate certificates for new devices

```bash
aws cloudformation create-stack --stack-name "devopstar-iot-cvm" \
    --template-body file://aws-iot-cvm/iot-cvm.yaml \
    --parameters file://aws-iot-cvm/iot-cvm-params.json \
    --capabilities CAPABILITY_IAM
```

Get details, including your API Endpoint for adding new IoT devices

```bash
aws cloudformation describe-stacks --stack-name "devopstar-iot-cvm" \
    --query 'Stacks[0].Outputs[?OutputKey==`RequestUrlExample`].OutputValue' \
    --output text

# https://xxxxxxxx.execute-api.us-east-1.amazonaws.com/LATEST/getcert?serialNumber=value1&deviceToken=value2
```

Create a new Item in DynamoDB for your device by replacing:

- **devopstar-accl-01**: With your desired name for the device
- **secret_key**: Something secret :)

```bash
aws dynamodb put-item \
    --table-name iot-cvm-device-info \
    --item '{"deviceToken":{"S":"secret_key"},"serialNumber":{"S":"devopstar-accl-01"}}'
```

Now make a request with the URL you got from the API gateway. Save the results to a file `config/iot-key.json`

```bash
https://xxxxxxxx.execute-api.us-east-1.amazonaws.com/LATEST/getcert?serialNumber=devopstar-iot-btn-01&deviceToken=secret_key
```

You'll be returned a json response:

```json
{
    "certificateArn": "arn:aws:iot:us-east-1:<account-id>:cert/009ff6ee0.........",
    "certificateId": "009ff6ee092e......",
    "certificatePem": "-----BEGIN CERTIFICATE-----\nMIIDWTCCAkGgAwIBAgIUZiIgLi......-----END CERTIFICATE-----\n",
    "keyPair": {
        "PublicKey": "-----BEGIN PUBLIC KEY-----\nMIIBIjANBgkqhkiG9w0BAQEFAAO.......-----END PUBLIC KEY-----\n",
        "PrivateKey": "-----BEGIN RSA PRIVATE KEY-----\nMIIEpAIBAAKCAQ........-----END RSA PRIVATE KEY-----\n"
    },
    "RootCA": "-----BEGIN CERTIFICATE-----\r\nMIIE0zCCA7ugAwIBAgIQGNrRniZ96Lt........-----END CERTIFICATE-----"
}
```

Place the outputs for each of the two fields below into new files in [aws-iot-esp8266/aws/certs](aws-iot-esp8266/aws/certs)

- **aws-iot-esp8266/aws/certs/certificate.pem.crt**: certificatePem
- **aws-iot-esp8266/aws/certs/private.pem.key**: keyPair.PrivateKey

*Annoyingly you'll have to remove the newline delimiters with actual newlines. I usually use a \\n -> \n regular expression find and replace in VSCode*

### Firmware Flashing

The specifics are outlined in [aws-iot-esp8266/README.md](aws-iot-esp8266/README.md)

## AWS Sumerian

### IoT Thing Cognito Identity

We'll use a nice script that was provided by the [Amazon Cognito Setup guide](https://docs.sumerian.amazonaws.com/tutorials/create/beginner/aws-setup/index.html). The CloudFormation template can be run using the following command

```bash
aws cloudformation create-stack \
    --stack-name "AmazonSumerianIotDataTutorialStack" \
    --template-body file://aws-cf/iot-thing-tutorial-template.yml \
    --capabilities CAPABILITY_IAM
```

Grab the Cognito User Pool ID using the following

```bash
aws cloudformation describe-stacks --stack-name "AmazonSumerianIotDataTutorialStack" \
    --query 'Stacks[0].Outputs[?OutputKey==`CognitoIdentityPoolID`].OutputValue' \
    --output text
```
