const express = require('express');
const { MongoClient } = require('mongodb');
const bodyParser = require('body-parser');
const cors = require('cors');
const multer = require('multer');
const path = require('path');
const fs = require('fs');
const moment = require('moment-timezone'); 

// Initialize Express
const app = express();
const PORT = process.env.PORT || 3000;
const uri = "mongodb+srv://sheikhabdulbaseer786:yoBFfmoORB3QEpjM@cluster0.hiywggw.mongodb.net/?retryWrites=true&w=majority&appName=Cluster0";

// Ensure 'tmp' folder exists
const tmpDir = path.join(__dirname, 'tmp');
if (!fs.existsSync(tmpDir)) {
  fs.mkdirSync(tmpDir);
}

// Middleware
app.use(bodyParser.json());
app.use(cors());
app.use(express.static('public')); // Serve frontend files

// Set up multer to save firmware as firmware.bin in the tmp folder
const storage = multer.diskStorage({
  destination: (req, file, cb) => {
    cb(null, tmpDir);  // Save in the tmp folder
  },
  filename: (req, file, cb) => {
    cb(null, 'firmware.bin');  // Always save as firmware.bin
  }
});
const upload = multer({ storage });

// POST endpoint to receive sensor data
app.post('/addReading', async (req, res) => {
  const { temperature, humidity } = req.body;

  if (temperature == null || humidity == null) {
    return res.status(400).send("Missing temperature or humidity");
  }

  const client = new MongoClient(uri);
  try {
    await client.connect();
    const db = client.db("DHT11");
    const collection = db.collection("dht_readings");

    const newReading = {
      temperature: parseFloat(temperature),
      humidity: parseFloat(humidity),
      timestamp: moment().tz("Asia/Karachi").toDate()  // Converts to Pakistan time
   };

    await collection.insertOne(newReading);
    res.status(200).send("Reading stored successfully");
  } catch (err) {
    console.error(err);
    res.status(500).send("Error saving to database");
  } finally {
    await client.close();
  }
});

// GET endpoint to return sensor data as JSON
app.get('/readings', async (req, res) => {
  const client = new MongoClient(uri);
  try {
    await client.connect();
    const db = client.db("DHT11");
    const collection = db.collection("dht_readings");

    const latestReadings = await collection.find().sort({ timestamp: -1 }).limit(100).toArray();
    res.json(latestReadings.reverse()); // oldest to newest
  } catch (err) {
    console.error(err);
    res.status(500).send("Error fetching data from MongoDB");
  } finally {
    await client.close();
  }
});

// Upload endpoint to handle firmware upload
app.post('/api/upload', upload.single('firmware'), (req, res) => {
  if (!req.file) {
    return res.status(400).json({ success: false, message: "No file uploaded" });
  }
  console.log(`Uploaded firmware: ${req.file.filename}`);
  res.status(200).json({ success: true, message: "Firmware uploaded successfully!" });
});

// Endpoint to serve the firmware to ESP32
app.get('/firmware', (req, res) => {
  const filePath = path.join(__dirname, 'tmp', 'firmware.bin');
  if (fs.existsSync(filePath)) {
    res.download(filePath);
  } else {
    res.status(404).send("Firmware file not found");
  }
});

// Start the server
app.listen(PORT, () => {
  console.log(`🌐 Server running at http://localhost:${PORT}`);
});
