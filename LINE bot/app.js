import line from '@line/bot-sdk';
import express from 'express';
import ngrok from 'ngrok';
import axios from 'axios';
import mqtt from 'mqtt';
import dotenv from 'dotenv';
import { initializeApp } from "firebase/app";
import { getFirestore } from "firebase/firestore";
import { doc, getDoc, setDoc } from "firebase/firestore";
dotenv.config()

var mqttClientStatus = mqtt.connect(process.env.HIVEMQ_BROKER);
mqttClientStatus.on('connect', () => {
  console.log('HIVEMQ connected Status');
  mqttClientStatus.subscribe([process.env.SENSOR_TOPIC_STATUS], () => {
    console.log("Topic subscribed Status");
  });
});

var mqttClientHumid = mqtt.connect(process.env.HIVEMQ_BROKER);
mqttClientHumid.on('connect', () => {
  console.log('HIVEMQ connected Humid');
  mqttClientHumid.subscribe([process.env.SENSOR_TOPIC_HUMID], () => {
    console.log("Topic subscribed Humid");
  });
});

//line-bot
const config = {
  channelAccessToken: process.env.CHANNEL_ACCESS_TOKEN,
  channelSecret: process.env.CHANNEL_SECRET,
};

const client = new line.Client(config);
const app = express();

//My Variables
let currentDataHumid = "";
let statusCount = 0;
let statusCountFull = Math.floor(statusCount / 2);

let date = getTodayDate();
let selectedDate = getYesterdayDate();
let nameDocumentDate = date.split('/').join('.')
let nameDocumentSelectedDate = selectedDate.split('/').join('.')
let json_date = { date: date, move_count: statusCountFull };

//action MQTT
mqttClientStatus.on('message', (topic, payload) => {
  console.log('Received Message:', topic, payload.toString())
  statusCount += 1;
  statusCountFull = Math.floor(statusCount / 2);
  date = getTodayDate();
  json_date = { date: date, move_count: statusCountFull };
  setData("sensors", nameDocumentDate, json_date);

});

mqttClientHumid.on('message', (topic, payload) => {
  console.log('Received Message:', topic, payload.toString())
  currentDataHumid = JSON.parse(payload.toString());

});

app.post('/callback', line.middleware(config), (req, res) => {
  Promise
    .all(req.body.events.map(handleEvent))
    .then((result) => res.json(result))
    .catch((err) => {
      console.error(err);
      res.status(500).end();
    });
});

// Firebase
const firebaseConfig = {
  apiKey: "AIzaSyD72xtEOEov0vijVOxbQZMk0NeKXo0HadQ",
  authDomain: "cn466-project.firebaseapp.com",
  projectId: "cn466-project",
  storageBucket: "cn466-project.appspot.com",
  messagingSenderId: "118172030971",
  appId: "1:118172030971:web:f9e3eebe828cc5783c9d5e",
  measurementId: "G-REZ1SMYB4M"
};

const firebaseApp = initializeApp(firebaseConfig);
const db = getFirestore();

async function setData(nameCollection, nameDocument, data) {
  try {
    await setDoc(doc(db, nameCollection, nameDocument), data);
    console.log("Document written with Date: ", nameDocument);
  } catch (e) {
    console.error("Error adding document: ", e);
  }
}

//My Function
function getTodayDate() {
  var today = new Date();

  var todayMonth = today.getMonth() + 1;
  if (todayMonth < 10) {
    todayMonth = "0" + (today.getMonth() + 1);
  }

  var todayDate = today.getDate();
  if (todayDate < 10) {
    todayDate = "0" + today.getDate()
  }

  var date = todayMonth + '/' + todayDate + '/' + today.getFullYear();

  return date;
}

function getYesterdayDate() {
  var today = new Date();
  var yesterday = new Date(today);

  yesterday.setDate(yesterday.getDate() - 1);

  var yesterdayMonth = yesterday.getMonth() + 1;
  if (yesterdayMonth < 10) {
    yesterdayMonth = "0" + (yesterday.getMonth() + 1);
  }

  var yesterdayDate = yesterday.getDate();
  if (yesterdayDate < 10) {
    yesterdayDate = "0" + yesterday.getDate();
  }

  var selectedDate = yesterdayMonth + '/' + yesterdayDate + '/' + yesterday.getFullYear();

  return selectedDate;
}

//Start Event
//อ่านหน้าเว็บจาก folder static
app.use(express.static('static'));

async function handleEvent(event) {

  var eventText = event.message.text.toLowerCase();
  console.log(event.message.text);

  date = getTodayDate();
  let statusCountFull = Math.floor(statusCount / 2);
  nameDocumentDate = date.split('/').join('.');
  nameDocumentSelectedDate = selectedDate.split('/').join('.');

  //แสดง Status และ Humidity
  if (eventText == "status") {
    return client.replyMessage(event.replyToken, { type: 'text', text: "จำนวนการเปิด/ปิดลิ้นชักในวันนี้: " + statusCountFull });
  }
  else if (eventText == "humidity") {
    return client.replyMessage(event.replyToken, { type: 'text', text: "ความชื้นปัจจุบัน: " + currentDataHumid.humidity });
  }
  else if ((eventText.charAt(2) == '/') && (eventText.charAt(5) == '/')) {
    selectedDate = eventText;
    nameDocumentSelectedDate = selectedDate.split('/').join('.');
    return client.replyMessage(event.replyToken, { type: 'text', text: "วันที่เลือก: " + selectedDate });
  }
  else if (eventText == "selected status") {
    try {
    const docRef = doc(db, "sensors", nameDocumentSelectedDate);
    const docSnap = await getDoc(docRef);

    let getSelectedDate = docSnap.data().date;
    let getSelectedMoveCount = docSnap.data().move_count;
    return client.replyMessage(event.replyToken, { type: 'text', text: "วันที่เลือก: " + getSelectedDate + "\nจำนวนการเปิด/ปิดลิ้นชัก: " + getSelectedMoveCount });
    } catch {
      return client.replyMessage(event.replyToken, { type: 'text', text: "วันที่เลือก: " + selectedDate + "\nจำนวนการเปิด/ปิดลิ้นชัก: " + 0 });
    }
  }
  else if (eventText == "date") {
    return client.replyMessage(event.replyToken, { type: 'text', text: "วันนี้: " + date });
  }
  else if (eventText == "selected date") {
    return client.replyMessage(event.replyToken, { type: 'text', text: "วันที่เลือก: " + selectedDate });
  }

  // use reply API
  return client.replyMessage(event.replyToken, msg);
}

const port = process.env.PORT || 3000;
// async function start_ngrok() {
//   const url = await ngrok.connect(port);
//   await client.setWebhookEndpointUrl(url + '/callback');
//   console.log(url);
// }

// start_ngrok();

async function start_heroku() {
  const url = process.env.BASE_URL;
  console.log('Set LINE webhook at ' + url + '/callback');
  await client.setWebhookEndpointUrl(url + '/callback');
}

start_heroku()

app.listen(port, () => {
  console.log(`listening on ${port}`);
});