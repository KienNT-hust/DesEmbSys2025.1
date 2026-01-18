/*
const express = require('express');
const app = express();
const port = 3000;
const path = require('path');
const fs = require('fs'); // Thư viện đọc/ghi file của hệ thống

app.use(express.json());

// Thư mục chứa dữ liệu log (tự động tạo nếu chưa có)
const DATA_FOLDER = path.join(__dirname, 'data_logs');
if (!fs.existsSync(DATA_FOLDER)) {
    fs.mkdirSync(DATA_FOLDER);
}

// Hàm hỗ trợ: Lấy tên file theo ngày hiện tại (Ví dụ: 2026-01-09.json)
function getTodayFileName() {
    const today = new Date();
    const year = today.getFullYear();
    const month = String(today.getMonth() + 1).padStart(2, '0');
    const day = String(today.getDate()).padStart(2, '0');
    return path.join(DATA_FOLDER, `${year}-${month}-${day}.json`);
}

// Hàm hỗ trợ: Đọc dữ liệu từ file ngày hôm nay
function readTodayData() {
    const filePath = getTodayFileName();
    if (fs.existsSync(filePath)) {
        const fileContent = fs.readFileSync(filePath, 'utf8');
        try {
            return JSON.parse(fileContent);
        } catch (e) {
            return []; // Nếu file lỗi thì trả về rỗng
        }
    }
    return [];
}

// Hàm hỗ trợ: Lưu dữ liệu mới vào file ngày hôm nay
function saveToTodayFile(newData) {
    const filePath = getTodayFileName();
    let currentData = readTodayData(); // Đọc dữ liệu cũ
    currentData.push(newData);         // Thêm dữ liệu mới
    fs.writeFileSync(filePath, JSON.stringify(currentData, null, 2), 'utf8'); // Ghi đè lại file
}

// --- API Giao diện Web (Index.html) ---
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'index.html'));
});

// --- API lấy dữ liệu để vẽ biểu đồ ---
app.get('/api/data', (req, res) => {
    // Chỉ trả về dữ liệu của ngày hôm nay để vẽ biểu đồ Realtime
    const todayData = readTodayData();
    res.json(todayData);
});

// --- API Xuất Excel (CSV) ---
app.get('/download-excel', (req, res) => {
    const todayData = readTodayData();
    let csvContent = "Ngay,Gio,Nhiet Do,Do Am\n";
    todayData.forEach(row => {
        csvContent += `${row.date},${row.time},${row.temp},${row.hum}\n`;
    });
    res.header('Content-Type', 'text/csv'); 
    res.attachment('baocao_hom_nay.csv');
    res.send(csvContent);
});

// --- API nhận dữ liệu từ ESP32 ---
app.post("/", (req,res) => {
    const { temp, hum } = req.body;
    console.log(`Nhận: Temp=${temp}, Hum=${hum}`);

    if (temp && hum) {
        // Tạo đối tượng dữ liệu
        const dataPoint = {
            time: new Date().toLocaleTimeString('vi-VN'), // Giờ Việt Nam
            date: new Date().toLocaleDateString('vi-VN'), // Ngày Việt Nam
            temp: temp,
            hum: hum
        };

        // LƯU VÀO FILE NGAY LẬP TỨC
        saveToTodayFile(dataPoint);
    }

    res.json(1); // Trả về 1 cho ESP32 biết là OK
});

app.listen(port, () => {
    console.log(`Server đang chạy tại http://localhost:${port}`);
});
*/
const express = require("express");
const app = express();
const port = 3000;
const path = require("path");
const fs = require("fs");

app.use(express.json());

// ======================= LƯU FILE THEO NGÀY =======================
const DATA_FOLDER = path.join(__dirname, "data_logs");
if (!fs.existsSync(DATA_FOLDER)) fs.mkdirSync(DATA_FOLDER);

function getTodayFileName() {
  const d = new Date();
  const yyyy = d.getFullYear();
  const mm = String(d.getMonth() + 1).padStart(2, "0");
  const dd = String(d.getDate()).padStart(2, "0");
  return path.join(DATA_FOLDER, `${yyyy}-${mm}-${dd}.json`);
}

function readTodayData() {
  const filePath = getTodayFileName();
  if (!fs.existsSync(filePath)) return [];
  try {
    const content = fs.readFileSync(filePath, "utf8");
    const data = JSON.parse(content);
    return Array.isArray(data) ? data : [];
  } catch (e) {
    return [];
  }
}

function saveToTodayFile(newData) {
  const filePath = getTodayFileName();
  const currentData = readTodayData();
  currentData.push(newData);
  fs.writeFileSync(filePath, JSON.stringify(currentData, null, 2), "utf8");
}

// ======================= PARSE RAW LORA =======================
// raw: "ID=1;SEQ=25;T=24.7;H=68.2;ADC=664;SOIL=0;STATE=Dry"
function parseRaw(raw) {
  const obj = {};
  String(raw)
    .split(";")
    .forEach((pair) => {
      const i = pair.indexOf("=");
      if (i < 0) return;
      const k = pair.slice(0, i).trim().toUpperCase();
      const v = pair.slice(i + 1).trim();
      obj[k] = v;
    });

  // ép kiểu số
  const toNum = (x) => {
    const n = Number(x);
    return Number.isFinite(n) ? n : null;
  };

  return {
    id: toNum(obj.ID),
    seq: toNum(obj.SEQ),
    temp: toNum(obj.T),
    hum: toNum(obj.H),
    adc: toNum(obj.ADC),
    soil: toNum(obj.SOIL),
    state: obj.STATE ?? null,
  };
}

function nowVN() {
  return {
    time: new Date().toLocaleTimeString("vi-VN"),
    date: new Date().toLocaleDateString("vi-VN"),
    ts: new Date().toISOString(),
  };
}

// ======================= WEB =======================
app.get("/", (req, res) => {
  res.sendFile(path.join(__dirname, "index.html"));
});

// lấy data hôm nay (có filter node + limit)
app.get("/api/data", (req, res) => {
  let data = readTodayData();

  // filter node (1..3)
  const node = req.query.node ? Number(req.query.node) : null;
  if (node && Number.isFinite(node)) {
    data = data.filter((x) => Number(x.id) === node);
  }

  // limit (mặc định lấy 300 điểm cuối cho nhẹ)
  const limit = req.query.limit ? Number(req.query.limit) : 300;
  if (Number.isFinite(limit) && limit > 0 && data.length > limit) {
    data = data.slice(data.length - limit);
  }

  res.json(data);
});

// lấy bản ghi mới nhất (có thể theo node)
app.get("/api/latest", (req, res) => {
  let data = readTodayData();
  const node = req.query.node ? Number(req.query.node) : null;
  if (node && Number.isFinite(node)) {
    data = data.filter((x) => Number(x.id) === node);
  }
  res.json(data.length ? data[data.length - 1] : null);
});

// export CSV (Excel)
app.get("/download-excel", (req, res) => {
  const todayData = readTodayData();
  let csv = "Ngay,Gio,NodeID,SEQ,NhietDo,DoAm,Soil,ADC,State,RSSI,SNR,Raw\n";

  todayData.forEach((row) => {
    const rawSafe = (row.raw ?? "").toString().replaceAll('"', '""');
    csv += `${row.date ?? ""},${row.time ?? ""},${row.id ?? ""},${row.seq ?? ""},${row.temp ?? ""},${row.hum ?? ""},${row.soil ?? ""},${row.adc ?? ""},${row.state ?? ""},${row.rssi ?? ""},${row.snr ?? ""},"${rawSafe}"\n`;
  });

  res.header("Content-Type", "text/csv; charset=utf-8");
  res.attachment("baocao_hom_nay.csv");
  res.send(csv);
});

// ======================= API NHẬN DỮ LIỆU =======================

/*
=== CODE CŨ (kỉ niệm): nhận temp/hum trực tiếp ===
app.post("/", (req,res) => {
    const { temp, hum } = req.body;
    console.log(`Nhận: Temp=${temp}, Hum=${hum}`);

    if (temp && hum) {
        const dataPoint = {
            time: new Date().toLocaleTimeString('vi-VN'),
            date: new Date().toLocaleDateString('vi-VN'),
            temp: temp,
            hum: hum
        };
        saveToTodayFile(dataPoint);
    }

    res.json(1);
});
=== HẾT CODE CŨ ===
*/

// ✅ Endpoint mới khuyên dùng cho ESP32 gateway
// body: { raw: "...", rssi: -72, snr: 9.5 }
app.post("/api/uplink", (req, res) => {
  const { raw, rssi, snr, temp, hum } = req.body || {};
  const t = nowVN();

  let record = {
    ...t,
    raw: raw ?? null,
    rssi: Number.isFinite(Number(rssi)) ? Number(rssi) : null,
    snr: Number.isFinite(Number(snr)) ? Number(snr) : null,
    id: null,
    seq: null,
    temp: null,
    hum: null,
    soil: null,
    adc: null,
    state: null,
  };

  // 1) Nếu có raw -> parse raw
  if (raw) {
    const p = parseRaw(raw);
    record = { ...record, ...p };
  }
  // 2) Nếu không có raw mà có temp/hum (compat code cũ)
  else if (temp !== undefined || hum !== undefined) {
    const tt = Number(temp);
    const hh = Number(hum);
    record.temp = Number.isFinite(tt) ? tt : null;
    record.hum = Number.isFinite(hh) ? hh : null;
  } else {
    return res.status(400).json({ ok: false, msg: "missing raw or temp/hum" });
  }

  saveToTodayFile(record);
  console.log("Uplink:", record);

  // trả 1 cho hợp kiểu cũ bạn đang dùng
  res.json(1);
});

// route POST "/" vẫn nhận được (đỡ phải sửa ESP32 nếu lỡ POST "/")
app.post("/", (req, res) => {
  // forward sang /api/uplink
  req.url = "/api/uplink";
  app._router.handle(req, res);
});

app.listen(port, () => {
  console.log(`Server đang chạy tại http://localhost:${port}`);
  console.log(`LAN: dùng IP Wi-Fi trong ipconfig (ví dụ http://10.xxx.xxx.xxx:${port})`);
});
