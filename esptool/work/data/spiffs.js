import { execSync, spawn } from 'child_process';
import { promises as fs } from 'fs';
import { program } from 'commander';
import path from 'path';
import {
    fileURLToPath
} from 'url';
import { get } from "http";
import { type } from "os";
const __filename = fileURLToPath(
    import.meta.url);
const __dirname = path.dirname(__filename);
//console.log("DIRNAME",__dirname);


const doCmd = async (cmd) => new Promise((resolve, reject) => {
    //split cmd to cmd and params
    let cmdArr = cmd.split(' ');
    let cmdName = cmdArr[0];
    let params = cmdArr.slice(1);
    //console.log(cmdName, params)
    let n = spawn(path.resolve(__dirname,cmdName), params);
    n.stdout.on('data', (data) => {
        console.log(data.toString());
    });
    n.on('close', (code) => {
        if (code !== 0) {
            //console.log(`child process exited with code ${code}`);
            reject(code);
        } else {
            //console.log(`child process exited with code ${code}`);
            resolve(code);
        }
    });
    n.on('error', (err) => {
        console.error(err);
        reject(err);
    });
    /*
    n.on('exit', (code) => {
        console.log(`child process exited with code ${code}`);
        resolve(code);
    });
    */

}
);

let port="COM18"
let speed=921600

const readPartitionTable = async () => {
    let cmd = `esptool.exe --port ${port} --baud ${speed} read_flash 0x8000 0x1000 ./partition_table.bin`;
    await doCmd(cmd);
}

const treatWD = (dir) => process.cwd()+"/"+dir;

const parsePartitionTable = async () => {
    let data = await fs.readFile('./partition_table.bin');
    //parse partition_table.bin
    //treat it as binary file
    //search for 0xaa 0x50 0x01 0x82 sequence
    //if found, read the next 4 bytes as start address of spiffs and next 4 bytes as size of spiffs
    let startAddress = 0;
    let size = 0;
    for(let i=0;i<data.length-4;i++){
        if(data[i]===0xaa && data[i+1]===0x50 && data[i+2]===0x01 && data[i+3]===0x82){
            startAddress = data.readUInt32LE(i+4);
            size = data.readUInt32LE(i+8);
            break;
        }
    }
    console.log(`startAddress: 0x${startAddress.toString(16)}, size: 0x${size.toString(16)}`);
    return {startAddress, size}
}

const readSpiffs = async (file, startAddress, size) => {
    let cmd = `esptool.exe --port ${port} --baud ${speed} read_flash 0x${startAddress.toString(16)} 0x${size.toString(16)} ${file}`;
    await doCmd(cmd);
}

const writeSpiffs = async (file, startAddress, size) => {
    let cmd = `esptool.exe --port ${port} --baud ${speed} write_flash 0x${startAddress.toString(16)} ${file}`;
    await doCmd(cmd);
}

const makeSpiffs = async (dataDir, spiffsFile, size) => {
    let cmd = `mkspiffs.exe -c ${treatWD(dataDir)} ${treatWD(spiffsFile)} -p 256 -b 4096 -s ${size}`;
    await doCmd(cmd);
}

const unpackSpiffs = async (spiffsFile, dataDir) => {
    let cmd = `mkspiffs.exe -u ${treatWD(dataDir)} ${treatWD(spiffsFile)}`;
        await doCmd(cmd);
}

const listSpiffs = async (spiffsFile) => {
    let cmd = `mkspiffs.exe -l ${treatWD(spiffsFile)}`;
    await doCmd(cmd);
}

const main = async () => {
    // call esptool.exe to read ESP partition table: esptool.exe --port COM18 read_flash 0x8000 0x1000 ./partition_table.bin
    await readPartitionTable();

    let data = await fs.readFile('./partition_table.bin');

    //parse partition_table.bin
    //treat it as binary file
    //search for 0xaa 0x50 0x01 0x82 sequence
    //if found, read the next 4 bytes as start address of spiffs and next 4 bytes as size of spiffs
    let startAddress = 0;
    let size = 0;
    for(let i=0;i<data.length-4;i++){
        if(data[i]===0xaa && data[i+1]===0x50 && data[i+2]===0x01 && data[i+3]===0x82){
            startAddress = data.readUInt32LE(i+4);
            size = data.readUInt32LE(i+8);
            break;
        }
    }
    console.log(`startAddress: 0x${startAddress.toString(16)}, size: 0x${size.toString(16)}`);
    //download spiffs.bin
    //await readSpiffs("./spiffs.bin", startAddress, size);

    await unpackSpiffs("./spiffs.bin", "./dataFromEsp");

    //make spiffs image from "./data" by mkspiffs.exe - mkspiffs -c data data.spiffs -p 256 -b 4096 -s ${size}
    await makeSpiffs("./data", "./data.spiffs", size);

    //flash it back to ESP32 by esptool.exe --port COM18 write_flash 0x${startAddress.toString(16)} ./data.spiffs
    //await writeSpiffs("./data.spiffs", startAddress, size);
}

//main().then(()=>{process.exit(0)}).catch((e)=>{console.error(e);process.exit(1)});

const prog = program
//console.log(program)

prog
 .version('0.0.1')
 .description('A tool to read/write spiffs image from/to ESP32')

prog 
 .command('read', { isDefault: true })
    .description('read spiffs directory from ESP32')
    .action(async () => {
        // call esptool.exe to read ESP partition table: esptool.exe --port COM18 read_flash 0x8000 0x1000 ./partition_table.bin
        await readPartitionTable();
        let part = await parsePartitionTable();
        //download spiffs.bin
        await readSpiffs(prog.opts().file, part.startAddress, part.size);
        await unpackSpiffs(prog.opts().file, prog.opts().data);
    });

prog.command('write')
    .description('write spiffs directory to ESP32')
    .action(async () => {
        // call esptool.exe to read ESP partition table: esptool.exe --port COM18 read_flash 0x8000 0x1000 ./partition_table.bin
        await readPartitionTable();
        let part = await parsePartitionTable();
        //make spiffs image from "./data" by mkspiffs.exe - mkspiffs -c data data.spiffs -p 256 -b 4096 -s ${size}
        await makeSpiffs(prog.opts().data, prog.opts().file, part.size);
        //flash it back to ESP32 by esptool.exe --port COM18 write_flash 0x${startAddress.toString(16)} ./data.spiffs
        await writeSpiffs(prog.opts().file, part.startAddress, part.size);
    });    

prog.command("unpack")
    .description('unpack spiffs image')
    .action(async () => {
        await unpackSpiffs(prog.opts().file, prog.opts().data);
    });

prog.command("part")
    .description('read partition table')
    .action(async () => {
        await readPartitionTable();
        let part = await parsePartitionTable();
        console.log(part);
    });

prog.command("make")
    .description('make spiffs image')
    .action(async () => {
        await makeSpiffs(prog.opts().data, prog.opts().file, part.size);
    });

prog.command("list")
    .description('list spiffs image')
    .action(async () => {
        await listSpiffs(prog.opts().file);
    });

prog
   .option('-p, --port <port>', 'COM port')
    .option('-s, --speed <speed>', 'baud rate', 921600)
    .option('-d, --data <data>', 'data directory', "./data")
    .option('-f, --file <file>', 'spiffs file', "./data.spiffs")
prog.parse();

