
% Copyright (c) 2002, 2017 Jens Keiner, Stefan Kunis, Daniel Potts
%
% This program is free software; you can redistribute it and/or modify it under
% the terms of the GNU General Public License as published by the Free Software
% Foundation; either version 2 of the License, or (at your option) any later
% version.
%
% This program is distributed in the hope that it will be useful, but WITHOUT
% ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
% FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
% details.
%
% You should have received a copy of the GNU General Public License along with
% this program; if not, write to the Free Software Foundation, Inc., 51
% Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

% Test script of class nfft for spatial dimension d=4.
clear all;

M=65; % number of nodes
N1=24; % number of Fourier coefficients in first direction
N2=32; % number of Fourier coefficients in second direction
N3=30; % number of Fourier coefficients in third direction
N4=10; % number of Fourier coefficients in forth direction
N=[N1;N2;N3;N4];

x=rand(M,4)-0.5; %nodes

% Initialisation
plan=nfft(4,N,M); % create plan of class type nfft
%n=2.^(ceil(log(N)/log(2))+1);
%plan=nfft(4,N,M,n,8,bitor(PRE_PHI_HUT,bitor(PRE_PSI,NFFT_OMP_BLOCKWISE_ADJOINT)),FFTW_ESTIMATE); % use of nfft_init_guru

plan.x=x; % set nodes in plan and perform precomputations

% NFFT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

fhat=rand(N1,N2,N3,N4); % Fourier coefficients
fhatv=fhat(:);

% Compute samples with NFFT
plan.fhat=fhatv; % set Fourier coefficients
tic
nfft_trafo(plan); % compute nonequispaced Fourier transform
fprintf('Time nfft_trafo:   %g sec\n',toc)
f1=plan.f; % get samples

% Compute samples direct
tic
k1=-N1/2:N1/2-1;
k2=-N2/2:N2/2-1;
k3=-N3/2:N3/2-1;
k4=-N4/2:N4/2-1;
[K1,K2,K3,K4]=ndgrid(k1,k2,k3,k4);
k=[K1(:), K2(:), K3(:), K4(:)];
clear K1 K2 K3 K4;
f2=zeros(M,1);
for j=1:M
	f2(j)=sum( fhatv.*exp(-2*pi*1i*(k*x(j,:).')) );
end %for
fprintf('Time direct trafo: %g sec\n',toc)

% Compare results
fprintf('Maximum error of trafo = %g\n',max(abs(f1-f2)))

% Adjoint NFFT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Computation with NFFT
tic
nfft_adjoint(plan);
fprintf('Time nfft_adjoint:   %g sec\n',toc)
fhat1=plan.fhat;

% Direct computation
tic
fhat2=zeros(prod(N),1);
for j=1:prod(N)
	fhat2(j)=sum( plan.f.*exp(2*pi*1i*(x*k(j,:).')) );
end %for
fprintf('Time direct adjoint: %g sec\n',toc)

% Compare results
fprintf('Maximum error of adjoint = %g\n',max(abs(fhat1-fhat2)))
