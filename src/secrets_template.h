#ifndef SECRETS_H
#define SECRETS_H

// Paste your AWS IoT Endpoint here.
// It can be found in the Settings page in the AWS IoT Console. 
const char *aws_server = "tcp://your-aws-endpoint-here";

// Change the following file names as per the names given when sending the
// certificates to SIM7600 module.

char *cacert = "Amazon-Root-Certificate-Filename";
char *clientcert = "Thing-Certificate-Filename";
char *clientkey = "Private-Key-Filename";

#endif