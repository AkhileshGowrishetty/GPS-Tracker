# AWS IoT Core Wiki

Documentation for the AWS IoT implemetation in Tracker project.

> References:  
> 1. [AWS Compute Blog - Building an AWS IoT Core device using AWS Serverless and an ESP32](https://aws.amazon.com/blogs/compute/building-an-aws-iot-core-device-using-aws-serverless-and-an-esp32/)
> 1. [AWS Docs - Tutorial: Storing device data in a DynamoDB table](https://docs.aws.amazon.com/iot/latest/developerguide/iot-ddb-rule.html)
---

**Table of Contents**

1. [Setting up IoT Core](#1-setting-up-iot-core)
    1. Registering a New Thing
    1. Creating a Policy
    1. Attaching the Policy
1. [Setting up Rule to insert received Data to DynamoDB](#2-setting-up-rule-to-insert-received-data-to-dynamodb)
    1. Creating a New Rule and Setting up Actions in the Rule

---

## 1. Setting up IoT Core
1. Registering a New Thing:  
    -  In the AWS Console, a Thing has to be registered before connecting to the MQTT broker. Firstly, we have to create in New Thing. Choose 'Things' in the console and choose **Register a New Thing**. Create a single thing.  
    - The thing can be named anything. The remaining fields can be left empty. Click on **Next**.  
    - Choose **Create Certificate**. Make sure to Download the *thing cert*, *private key* and *Amazon Root CA1* certificates. Save them somewhere secure. They will be needed to connect to the AWS IoT.
        > In this project, these certificates are sent to SIM7600 Module. The instructions can be found [here](./Setting_Up_SIM7600.md#2-download-certificates).
    - Click on **Activate**, **Attach a policy**.  
        > A policy is an object in AWS that, when associated with an identity or resource, defines their permissions. - [AWS Docs](https://docs.aws.amazon.com/IAM/latest/UserGuide/access_policies.html)
    - Skip adding a policy as of now. 
    - Choose **Register Thing**.

1. Creating a Policy:
    - In the AWS Console, choose **Secure, Policies, Create a policy**.
    - Name the policy. Choose the **Advanced** tab.
    - The following policy template can be pasted. In the template, replace the following:
        - **REGION:ACCOUNT_ID** with your **AWS region** and **AWS Account ID**.    
        - **ThingName** with the name of the Thing that was registered in previous step.
        - **SubTopic** with the topic which the Thing can subscribe.
        - **PubTopic** with the topic which the Thing can publish to.  
        ```json
        {
            "Version": "2012-10-17",
            "Statement": [
                {
                "Effect": "Allow",
                "Action": "iot:Connect",
                "Resource": "arn:aws:iot:REGION:ACCOUNT_ID:client/ThingName"
                },
                {
                "Effect": "Allow",
                "Action": "iot:Subscribe",
                "Resource": "arn:aws:iot:REGION:ACCOUNT_ID:topicfilter/SubTopic"
                },
                {
                "Effect": "Allow",
                "Action": "iot:Receive",
                "Resource": "arn:aws:iot:REGION:ACCOUNT_ID:topic/SubTopic"
                },
                {
                "Effect": "Allow",
                "Action": "iot:Publish",
                "Resource": "arn:aws:iot:REGION:ACCOUNT_ID:topic/PubTopic"
                }
            ]
        }
        ```
    - Choose **Create**.

1. Attaching the Policy:
    - In the AWS Console, choose **Secure, Certification**.
    - Select the one that was created for the registered device and choose **Actions, Attach Policy**.
    - Choose the previously created policy and click on **Attach**.

---

## 2. Setting up Rule to insert received Data to DynamoDB
Prior to Rule creation, a Table in DynamoDB must be created. It will have a primary key, sort key and data as columns. For this example, it is expected that a Table has a primary key of *Number* type and sort key of *String* type. The primary key will store the **Timestamp** and the sort key will store the **Topic** of the MQTT message.  
### Creating a New Rule and Setting up Actions in the Rule:  
- In the AWS Console, choose **Act, Rules**.
    - Click on **Create**.
    - In the top part of **Create a rule**:
        - In **Name**, enter the rule's name.
        - The rule can be described in the **Description**.
    - In the **Rule query statement** of **Create a rule**:
        - Select `2016-03-23` in **Using SQL version**.
        - In the **Rule query statement** edit box, paste the following. The statement implies that all the data from the topic to which data is published (**PubTopic**) is selected.
            ```sql
            SELECT * FROM "PubTopic"
            ```
    - In **Set one or more actions**:
        - Choose **Add action**, to open a list of rule actions.
        - In **Select an action**, choose **Insert a message into a DynamoDB table**.
        - Choose **Configure action** to open the selected action's configuration page.
    - In **Configure action**:
        - Choose the name of the DynamoDB Table to use in **Table Name**.
        - In **Partition key value**, enter `${timestamp()}`.
        - In **Sort key value**, enter `${topic()}`.
        - In **Write message data to this column**, enter `device_data`. This will create the `device_data` column in the DynamoDB Table.
        - The **Operation** field can be left blank.
        - In **Choose or create a role to grant AWS IoT access to perform this action**, choose **Create Role** if a new role has to be created or choose and existing role with the policies that allows to put an item in DynamoDB Tables.
        - In **Create a new role** enter the name of the role, and choose **Create role**.
        - Choose **Add action** at the bottom of **Configure action**.
        - Choose **Create rule** at the bottom of **Create a rule** to create the rule.