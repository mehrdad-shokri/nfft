function T = readTestcase(file)
% READTESTCASE Read fastsumS2.c testcase results from file
%   READTESTCASE(FILE) reads fastsumS2.c testcase results from the file FILE.
%   The testcase results are returned as a cell vector containing the data for
%   each individual testcase as a structure with the following fields:
%   - USENFSFT If true, the NFSFT algorithm has been used, otherwise the direct
%     NDSFT algorithm
%   - USENFFT If true, the NFFT algorithm has been used, otherwise the direct
%     NDFT algorithm (undefined if USENFSFT is false),
%   - CUTOFF The NFFT cut-off parameter (undefined if USENFFT is false)
%   - USEFPT If true, the fast polynomial transform has been used, otherwise the
%     direct polynomial transform algorithm (undefined if USENFSFT is false),
%   - THRESHOLD The fast polynomial transform threshold parameter (undefined if
%     USEFPT is false),
%   - kernel The kernel function used (0 = Abel-Poisson kernel, 1 =
%     singularity kernel, 2 = locally supported kernel, 3 = spherical Gaussian
%     kernel),
%   - parameters A m x n matrix containing the kernel function parameters,
%     m is the number of parameter sets and n is the number of kernel function
%     parameters (1 for Abel-Poisson, singularity and spherical Gaussian kernel,
%     2 for the locally supported kernel)
%   - bandwidths The vector of cut-off degrees for the approximation
%   - nodes The node sets.
%   - The result data

% $Id$
%
% Copyright (C) 2005 Jens Keiner
%
% This program is free software; you can redistribute it and/or modify
% it under the terms of the GNU General Public License as published by
% the Free Software Foundation; either version 2, or (at your option)
% any later version.
%
% This program is distributed in the hope that it will be useful,
% but WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
% GNU General Public License for more details.
%
% You should have received a copy of the GNU General Public License
% along with this program; if not, write to the Free Software Foundation,
% Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

% Read the number of testcases.
tc_max = fscanf(file,'%d',1);

% Create empty cell array for the testcases.
T = cell(tc_max,1);

% Cycle through all testcases.
for i = 1:tc_max

  % Create structure.
  t = struct('USENFSFT',[0],'USENFFT',[0],'CUTOFF',[0],'USEFPT',[0],...
    'THRESHOLD',[0],'kernel',[0],'parameters',{0},'bandwidths',[0],...
    'nodes',{0},'data',{0});

  % Read NFSFT usage flag.
  v = fscanf(file,'%d',1);
  if (v >= 1)

    % Set NFSFT usage flag in the structure.
    t.USENFSFT = [true];

    % Read NFFT usage flag.
    v = fscanf(file,'%d',1);

    if (v >= 1)

      % Set NFSFT usage flag in the structure.
      t.USENFFFT = [true];

      % Read the NFFT cut-off parameter.
      v = fscanf(file,'%d',1);
      t.CUTOFF = [v];

    else

      % Set NFSFT usage flag in the structure.
      t.USENFFFT = [false];

    end

    % Read FPT usage flag.
    v = fscanf(file,'%d',1);

    if (v >= 1)

      % Set NFSFT usage flag in the structure.
      t.USEFPT = [false];

    else

      % Set NFSFT usage flag in the structure.
      t.USEFPT = [false];

    end

    % Read the FPT threshold.
    v = fscanf(file,'%lf',1);
    t.THRESHOLD = [v];

  else

    % Set NFSFT usage flag in the structure.
    t.USENFSFT = [false];

  end

  % Read the kernel type.
  v = fscanf(file,'%d',1);
  t.kernel = [v];

  % Read the number of parameter sets.
  ip_max = fscanf(file,'%d',1);

  % Read the number of parameters.
  ipp_max = fscanf(file,'%d',1);

  % Create empty array for parameters.
  p = zeros(ipp_max,ip_max);

  % Read parameters.
  p = fscanf(file,'%lf',[ipp_max,ip_max]);

  % Transpose matrix to get dimensions right.
  t.parameters = p';

  % Read number of cut-off degrees
  bandwidths = fscanf(file,'%d',1);

  % Read the cut-off degrees
  m = fscanf(file,'%d',bandwidths);
  t.bandwidths = m;

  % Read number of node sets.
  ild_max = fscanf(file,'%d',1);

  % Read node sets.
  nodes = zeros(ild_max,5);
  for j=1:ild_max
    % Read first three parameters.
    nodes(j,1) = fscanf(file,'%d',1);
    nodes(j,2) = fscanf(file,'%d',1);
    nodes(j,3) = fscanf(file,'%d',1);
    % Check for for parameters.
    if (nodes(j,3) == 1)
      % Read two more parameters.
      nodes(j,4) = fscanf(file,'%d',1);
      nodes(j,5) = fscanf(file,'%d',1);
    end
  end

  % Assign node set matrix to data field in structure.
  t.nodes = nodes;

  % Create cell array for result data
  datacell = {ip_max,ild_max};

  % For each parameter set and node set combination read the result data for
  % every cut-off degree into a matrix.
  for j=1:ip_max
    for k=1:ild_max
      data = fscanf(file,'%e',[6,bandwidths]);
      datacell{j,k} = data';
    end
  end

  % Assign cell array to data field in structure.
  t.data = datacell;

  % Assign testcase structure to field in cell array.
  T{i} = t;

end

% End of the function
return;