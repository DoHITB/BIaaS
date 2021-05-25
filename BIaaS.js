/*
 * BIaaS.js
 *
 * @Version 1.0 (19/05/2021)
 * @Requires jquery
 * @Author DoHITB
 */

///// MAIN VARIABLES /////
//  Result of operation
var BIaaSResult;

//  Messages
var BIaaSMessages;

//  FileName
var BIaaSFileName;

//  operation status
var BIaaSStatus;

//  beacon tracing (will be false while developing)
var isBIaaSLoaded = false;
///// MAIN VARIABLES /////

//BIaaS function: perform AJAX operation
function BIaaS(bdata, bsucc){
  $.ajax({
    url: 'https://dohitb.ddns.net/BIaaS.calc',
    data: bdata,
    success: bsucc,
    dataType: 'json'
  });
}

//BIaaS_makeoperatoin: main entrance for operatin
function BIaaS_makeOperation(fn, op, op1, op2){
  //operation status: pending
  BIaaSStatus = false;

  let bmor = true;

  bmor = BIaaS_checkFN(fn)        &&
         BIaaS_checkOperation(op) &&
         BIaaS_checkOp(op1)       &&
         BIaaS_checkOp(op2);

  if(bmor === true){
    let bmod = {'kind' : 'makeOperation', 'fileName' : fn, 'operation' : op, 'op1' : op1, 'op2': op2};
    let bmof = BIaaS_getResult;

    //set global fileName
    BIaaSFileName = fn;

    //call AJAX
    BIaaS(bmod, bmof);
  }else{
    //operation status: done (with error message on BIaaSMessages)
    BIaaSStatus = true;
    BIaaSResult = "";
    return false;
  }

  return true;
}

//BIaaS_getResult: tries to retreive a result
function BIaaS_getResult(data){
  console.log(data);
  let bgrd = data;

  if(bgrd.error){
    //there's some error. We're done
    BIaaSMessages = bgrd.error;
    BIaaSResult = "";
    BIaaSStatus = true;

    return false;
  }else{
    let bgrr = true;

    bgrr = BIaaS_checkFN(BIaaSFileName);

    if(bgrr){
      let bgrd = {'kind' : 'getResult', 'fileName' : BIaaSFileName};
      let bgrf = BIaaS_putResult;

      BIaaS(bgrd, bgrf);
    }else{
      BIaaSResult = "";
      BIaaSStatus = true;
      return false;
    }
  }

  return true;
}

//BIaaS_putResult: tries to get the actual result
function BIaaS_putResult(data){
  console.log(data);
  let bprd = data;

  if(bprd.error){
    if(bprd.error == "operation_in_progress"){
      //BI still didn't operate. Just wait one second and retry getResult
      BIaaS_wait(1000);
    }else{
      //an actual error. We're don
      BIaaSMessages = bprd.error;
      BIaaSResult = "";
      BIaaSStatus = true;

      return false;
    }
  }else{
    //BI did the operation before entering on the function
    BIaaSResult = bprd.result;
    BIaaSMessages = "";
    BIaaSStatus = true;

    return true;
  }

  return true;
}

//BIaaS_checkFN: checks fileName
function BIaaS_checkFN(fn){
  let bcfni = 0;

  if(fn.length > 126){
    BIaaSMessages = "FileName too long";
    return false;
  }

  for(bcfni = 0;bcfni < fn.length;bcfni++)
    if((fn.charCodeAt(bcfni) >= 48 && fn.charCodeAt(bcfni) <= 57) || /* '1' .. '9' */
       (fn.charCodeAt(bcfni) >= 65 && fn.charCodeAt(bcfni) <= 90) || /* 'a' .. 'z' */
       (fn.charCodeAt(bcfni) >= 97 && fn.charCodeAt(bcfni) <= 122)){ /* 'A' .. 'Z'*/
      continue;
    }else{
      BIaaSMessages = "Invalid FileName";
      return false;
    }

  return true;
}

//BIaaS_checkOperation: checks operation
function BIaaS_checkOperation(op){
  if(op.length != 1){
    BIaaSMessage = "operator too long";
    return false;
  }

  if(op.charAt(0) == '+' || op.charAt(0) == '-' || op.charAt(0) == '*' ||
     op.charAt(0) == '/' || op.charAt(0) == '^' || op.charAt(0) == 's')
    return true;

  BIaaSMessages = "Invalid operator";
  return false;
}

//BIaaS_checkOp: checks operand
function BIaaS_checkOp(op){
  let bcoi = 0;

  if(op.length > 4097){
    BIaaSMessages = "Operand too long";
    return false;
  }

  for(bcoi = 0;bcoi < op.length;bcoi++)
    if(op.charCodeAt(bcoi) == 45 ||                             /* '-' */
       op.charCodeAt(bcoi) == 44 ||                             /* ',' */
      (op.charCodeAt(bcoi) >= 48 && op.charCodeAt(bcoi) <= 57)) /* '1' .. '9' */
      continue;
    else{
      BIaaSMessages = "Invalid operand";
      return false;
    }

  return true;
}

async function BIaaS_wait(ms){
  //sleep execution, then mock up a getResult
  await BIaaS_sleep(ms);
  BIaaS_getResult({'kind' : 'retry'});
}

//aux function to sleep ms milliseconds
function BIaaS_sleep(ms){return new Promise(resolve => setTimeout(resolve, ms))};
