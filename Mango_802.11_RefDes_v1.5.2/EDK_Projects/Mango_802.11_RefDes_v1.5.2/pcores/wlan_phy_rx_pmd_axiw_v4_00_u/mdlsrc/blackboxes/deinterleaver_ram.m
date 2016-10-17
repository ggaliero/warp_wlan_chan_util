
function deinterleaver_ram(this_block)

  this_block.setTopLevelLanguage('Verilog');

  this_block.setEntityName('deinterleaver_ram');

  % System Generator has to assume that your entity  has a combinational feed through; 
  %   if it  doesn't, then comment out the following line:
  this_block.tagAsCombinational;

  this_block.addSimulinkInport('dina');
  this_block.addSimulinkInport('wea');
  this_block.addSimulinkInport('addra');
  this_block.addSimulinkInport('dinb');
  this_block.addSimulinkInport('web');
  this_block.addSimulinkInport('addrb');

  this_block.addSimulinkOutport('douta');
  this_block.addSimulinkOutport('doutb');

  douta_port = this_block.port('douta');
  douta_port.setType('UFix_4_0');
  doutb_port = this_block.port('doutb');
  doutb_port.setType('UFix_4_0');

  % -----------------------------
  if (this_block.inputTypesKnown)
    % do input type checking, dynamic output type and generic setup in this code block.

    if (this_block.port('dina').width ~= 32);
      this_block.setError('Input data type for port "dina" must have width=32.');
    end

    if (this_block.port('wea').width ~= 1);
      this_block.setError('Input data type for port "wea" must have width=1.');
    end

    if (this_block.port('addra').width ~= 9);
      this_block.setError('Input data type for port "addra" must have width=9.');
    end

    if (this_block.port('dinb').width ~= 32);
      this_block.setError('Input data type for port "dinb" must have width=32.');
    end

    if (this_block.port('web').width ~= 1);
      this_block.setError('Input data type for port "web" must have width=1.');
    end

    if (this_block.port('addrb').width ~= 9);
      this_block.setError('Input data type for port "addrb" must have width=9.');
    end

  end  % if(inputTypesKnown)
  % -----------------------------

  % -----------------------------
   if (this_block.inputRatesKnown)
     setup_as_single_rate(this_block,'clk','ce')
   end  % if(inputRatesKnown)
  % -----------------------------

    % (!) Set the inout port rate to be the same as the first input 
    %     rate. Change the following code if this is untrue.
    uniqueInputRates = unique(this_block.getInputRates);


  % Add addtional source files as needed.
  %  |-------------
  %  | Add files in the order in which they should be compiled.
  %  | If two files "a.vhd" and "b.vhd" contain the entities
  %  | entity_a and entity_b, and entity_a contains a
  %  | component of type entity_b, the correct sequence of
  %  | addFile() calls would be:
  %  |    this_block.addFile('b.vhd');
  %  |    this_block.addFile('a.vhd');
  %  |-------------

  %    this_block.addFile('');
  %    this_block.addFile('');
  this_block.addFile('blackboxes/dp_ram_wr_32b_rd_4b_2048b.ngc');
  this_block.addFile('blackboxes/deinterleaver_ram.v');

return;


% ------------------------------------------------------------

function setup_as_single_rate(block,clkname,cename) 
  inputRates = block.inputRates; 
  uniqueInputRates = unique(inputRates); 
  if (length(uniqueInputRates)==1 & uniqueInputRates(1)==Inf) 
    block.addError('The inputs to this block cannot all be constant.'); 
    return; 
  end 
  if (uniqueInputRates(end) == Inf) 
     hasConstantInput = true; 
     uniqueInputRates = uniqueInputRates(1:end-1); 
  end 
  if (length(uniqueInputRates) ~= 1) 
    block.addError('The inputs to this block must run at a single rate.'); 
    return; 
  end 
  theInputRate = uniqueInputRates(1); 
  for i = 1:block.numSimulinkOutports 
     block.outport(i).setRate(theInputRate); 
  end 
  block.addClkCEPair(clkname,cename,theInputRate); 
  return; 

% ------------------------------------------------------------

