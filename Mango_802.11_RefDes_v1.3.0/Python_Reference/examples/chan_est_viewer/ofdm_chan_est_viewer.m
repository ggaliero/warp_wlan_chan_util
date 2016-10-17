function varargout = ofdm_chan_est_viewer(varargin)
% ofdm_chan_est_viewer MATLAB code for ofdm_chan_est_viewer.fig
%      ofdm_chan_est_viewer, by itself, creates a new ofdm_chan_est_viewer or raises the existing
%      singleton*.
%
%      H = ofdm_chan_est_viewer returns the handle to a new ofdm_chan_est_viewer or the handle to
%      the existing singleton*.
%
%      ofdm_chan_est_viewer('CALLBACK',hObject,eventData,handles,...) calls the local
%      function named CALLBACK in ofdm_chan_est_viewer.M with the given input arguments.
%
%      ofdm_chan_est_viewer('Property','Value',...) creates a new ofdm_chan_est_viewer or raises the
%      existing singleton*.  Starting from the left, property value pairs are
%      applied to the GUI before ofdm_chan_est_viewer_OpeningFcn gets called.  An
%      unrecognized property name or invalid value makes property application
%      stop.  All inputs are passed to ofdm_chan_est_viewer_OpeningFcn via varargin.
%
%      *See GUI Options on GUIDE's Tools menu.  Choose "GUI allows only one
%      instance to run (singleton)".
%
% See also: GUIDE, GUIDATA, GUIHANDLES

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Mango 802.11 Reference Design - Channel Estimate Viewer Example
%  Copyright 2014 Mango Communications, Inc. - All Rights Reserved
%  Distributed under the WARP License: http://warpproject.org/license
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Edit the above text to modify the response to help ofdm_chan_est_viewer

% Last Modified by GUIDE v2.5 18-Mar-2014 11:01:04

% Begin initialization code - DO NOT EDIT
gui_Singleton = 1;
gui_State = struct('gui_Name',       mfilename, ...
                   'gui_Singleton',  gui_Singleton, ...
                   'gui_OpeningFcn', @ofdm_chan_est_viewer_OpeningFcn, ...
                   'gui_OutputFcn',  @ofdm_chan_est_viewer_OutputFcn, ...
                   'gui_LayoutFcn',  [] , ...
                   'gui_Callback',   []);
if nargin && ischar(varargin{1})
    gui_State.gui_Callback = str2func(varargin{1});
end

if nargout
    [varargout{1:nargout}] = gui_mainfcn(gui_State, varargin{:});
else
    gui_mainfcn(gui_State, varargin{:});
end
% End initialization code - DO NOT EDIT


% --- Executes just before ofdm_chan_est_viewer is made visible.
function ofdm_chan_est_viewer_OpeningFcn(hObject, eventdata, handles, varargin)
% This function has no output args, see OutputFcn.
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
% varargin   command line arguments to ofdm_chan_est_viewer (see VARARGIN)

% Choose default command line output for ofdm_chan_est_viewer
handles.output = hObject;

%%%%%%%
%Format the axes

%Formats specified here are only used until the fist plots are drawn
% Formats must be re-applied in cell selection callback below

%Format the chan est mag plot
handles.mag_plot_lims = [-32 31 -70 -20];
handles.phase_plot_lims = [-32 31 -3.2 +3.2];

ax = handles.axes_chan_mag;
set(ax, 'XLim', handles.mag_plot_lims(1:2));
set(ax, 'YLim', handles.mag_plot_lims(3:4));
xlabel(ax, 'Subcarrier Index');
ylabel(ax, 'Magnitude (dB)');
grid(ax, 'On');

%Format the chan phase plot
ax = handles.axes_chan_phase;
set(ax, 'XLim', handles.phase_plot_lims(1:2));
set(ax, 'YLim', handles.phase_plot_lims(3:4));
xlabel(ax, 'Subcarrier Index');
ylabel(ax, 'Phase (radians)');
grid(ax, 'On');


%%%%%%%%%%%%
%Load and pre-process the data
if(~isempty(varargin))
    log_filename = varargin{1};
else
    log_filename = 'np_rx_ofdm_entries.hdf5';
end

rx_entries = rx_data_from_hdf5(log_filename);

rx_t        = rx_entries.timestamp;
rx_g_bb_db  = rx_entries.g_bb_db;
rx_g_rf_db  = rx_entries.g_rf_db;
rx_fcs      = logical(rx_entries.fcs_result == 1);
rx_h        = rx_entries.chan_ests;
rx_pwr      = rx_entries.pwr;

%Convert chan ests to dB
rx_h_db = 10*log10(abs(rx_h));

%Set zero subcarrier magnitudes to sub-axis value
rx_h_db( real(rx_h) == 0) = handles.mag_plot_lims(3) - 5;

%Update the text box summary of the log contents
num_rx = numel(rx_t);
num_rx_good = sum(rx_fcs == 0);
num_rx_bad = sum(rx_fcs ~= 0);

info_txt = '';
info_txt = [info_txt sprintf('Log file: %s\n', log_filename)];
info_txt = [info_txt sprintf('Log Time Span (sec): %5.3f\n\n', 1e-6 * (max(rx_t) - min(rx_t)))];
info_txt = [info_txt sprintf('Rx Log Entries:    %6d\n', num_rx)];
info_txt = [info_txt sprintf(' FCS Good Entries: %6d (%2.2f%%)\n', num_rx_good, (100*num_rx_good/num_rx))];
info_txt = [info_txt sprintf(' FCS Bad Entries:  %6d (%2.2f%%)\n', num_rx_bad, (100*num_rx_bad/num_rx))];

set(handles.info_text_area, 'String', info_txt);

%Save data to the GUI data structure
handles.rx_h_db = rx_h_db;
handles.rx_h_phase = angle(rx_h);
handles.rx_h_phase(:, [1:6 33 58:63]) = NaN;

%Populate the table of Rx entries for user selection
idx_good = (rx_fcs == 0);
idx_bad = (rx_fcs ~= 0);

entry_table_data_all = [rx_t rx_pwr rx_g_rf_db rx_g_bb_db rx_fcs];
entry_table_data_good = entry_table_data_all(idx_good, :);
entry_table_data_bad = entry_table_data_all(idx_bad, :);

update_entry_table(handles, entry_table_data_all)

handles.entry_table_data_all  = entry_table_data_all;
handles.entry_table_data_good = entry_table_data_good;
handles.entry_table_data_bad  = entry_table_data_bad;

% Update handles structure
guidata(hObject, handles);
%%%%%%%%END INIT

% UIWAIT makes ofdm_chan_est_viewer wait for user response (see UIRESUME)
% uiwait(handles.figure1);

% --- Outputs from this function are returned to the command line.
function varargout = ofdm_chan_est_viewer_OutputFcn(hObject, eventdata, handles) 
% varargout  cell array for returning output args (see VARARGOUT);
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Get default command line output from handles structure
varargout{1} = handles.output;


% --- Executes when selected cell(s) is changed in entry_table.
function entry_table_CellSelectionCallback(hObject, eventdata, handles)
% hObject    handle to entry_table (see GCBO)
% eventdata  structure with the following fields (see UITABLE)
%	Indices: row and column indices of the cell(s) currently selecteds
% handles    structure with handles and user data (see GUIDATA)

% Remove duplicate row IDs
selection = eventdata.Indices(:,1);
selection = unique(selection);
update_chan_plots(selection, handles)


function update_chan_plots(sel_idx, handles)

%Extract the selected channel data
% Transpose here so dimensions of arguments to stairs() below match
h_mag = handles.rx_h_db(sel_idx, :).';
h_phase = handles.rx_h_phase(sel_idx, :).';
sc = [-32:31];

%Plot chan magnitude
ax = handles.axes_chan_mag;

h_p = stairs(ax, sc, h_mag);
set(h_p, 'LineWidth', 2);
set(ax, 'XLim', handles.mag_plot_lims(1:2));
set(ax, 'YLim', handles.mag_plot_lims(3:4));
xlabel(ax, 'Subcarrier Index');
ylabel(ax, 'Magnitude (dB)');
grid(ax, 'On');

%Plot chan phase
ax = handles.axes_chan_phase;

h_p = stairs(ax, sc, h_phase);
set(h_p, 'LineWidth', 2);
set(ax, 'XLim', handles.phase_plot_lims(1:2));
set(ax, 'YLim', handles.phase_plot_lims(3:4));
xlabel(ax, 'Subcarrier Index');
ylabel(ax, 'Phase (radians)');
grid(ax, 'On');

drawnow;

% --- Executes during object creation, after setting all properties.
function entry_table_CreateFcn(hObject, eventdata, handles)
% hObject    handle to entry_table (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called
colNames{1} = 'Timestamp (usec)';   colFmts{1} = 'numeric'; colWidths{1} = 125;
colNames{2} = 'Rx Power (dBm)';     colFmts{2} = 'numeric'; colWidths{2} = 85;
colNames{3} = 'RF Gain (dB)';       colFmts{3} = 'numeric'; colWidths{3} = 65;
colNames{4} = 'BB Gain (dB)';       colFmts{4} = 'numeric'; colWidths{4} = 65;
colNames{5} = 'FCS Bad';            colFmts{5} = 'numeric'; colWidths{5} = 50;

set(hObject, 'ColumnName', colNames);
set(hObject, 'ColumnWidth', colWidths);
set(hObject, 'ColumnFormat', colFmts);
set(hObject, 'ColumnEditable', false(1,length(colNames)));

function update_entry_table(handles, data_mat)
set(handles.entry_table, 'Data', data_mat);


% --- Executes on button press in button_show_good.
function button_show_good_Callback(hObject, eventdata, handles)
% hObject    handle to button_show_good (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
update_entry_table(handles, handles.entry_table_data_good)

% --- Executes on button press in button_show_bad.
function button_show_bad_Callback(hObject, eventdata, handles)
% hObject    handle to button_show_bad (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
update_entry_table(handles, handles.entry_table_data_bad)


% --- Executes on button press in button_show_all.
function button_show_all_Callback(hObject, eventdata, handles)
% hObject    handle to button_show_all (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
update_entry_table(handles, handles.entry_table_data_all)
