const formidable = require('formidable');

module.exports = (req, res) => {
  if (req.method !== 'POST') {
    return res.status(405).json({ message: 'Method Not Allowed' });
  }

  const form = new formidable.IncomingForm({ multiples: false });

  form.parse(req, (err, fields, files) => {
    if (err || !files.firmware) {
      return res.status(400).json({ success: false, message: "No file uploaded" });
    }

    // You could log info or forward file to external storage (S3, etc.)
    console.log("Firmware file received:", files.firmware.originalFilename);

    res.status(200).json({ success: true, message: "Firmware uploaded (not stored on Vercel)!" });
  });
};

export const config = {
  api: {
    bodyParser: false, // required for formidable to work
  },
};
