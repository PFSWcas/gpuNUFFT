function [res] = gridding3D_init(k,n,osf,sw,w)
%function [res] = gridding3D_init(k,n,osf,sw,w)
%
% w ... k-space data weighting, density compensation
%
coords = k;
[data_sector_idx, sector_centers,sector_dim] = assign_sectors(osf*n,sw,coords);

[v i] = sort(data_sector_idx);
v = v +1;
% calculate offset of data entries for each sector, starts with 0
% e.g. 
% data(1), data(2) and data(3) -> sector 1 
% data (4) -> sector 2 
% data (5) -> sector 3
% ...
% sector_data_cnt = [0,3,4,5..
% 
% means that data indices between array index 0 and 1 are assigned to
% sector 1, thus 1,2,3

sector_data_cnt = zeros(1,sector_dim+1);
cnt = 0;
max_cnt = 512;
offset=0;
sector_data_cnt(1) = 0;
for b=1:sector_dim+1
    while (cnt < length(v) && b == int32(v(cnt+1)))
        cnt = cnt +1;
	%check if sector is densely filled
        %and separate 
        if (cnt - sector_data_cnt(b+offset)) >= max_cnt 
            sector_data_cnt(b+1+offset)=cnt;
            sector_centers = [sector_centers(1:3*(b+offset)) sector_centers(3*(b+offset)-2:3*(b+offset)) sector_centers(3*(b+offset)+1:end)];
            offset = offset + 1;
        end
    end
    sector_data_cnt(b+1+offset)=cnt;
end
res.sector_data_cnt = sector_data_cnt;
sector_dim = sector_dim + offset;

%sector_data_cnt
% calculate indices of data elements in order to sort them
data_ind = i-1;
res.data_ind=data_ind+1;%[2*data_ind+1;2*data_ind+2];

% calculate indices of coord elements in order to sort them
coord_ind = i-1;
res.coord_ind = [3*coord_ind+1;
             3*coord_ind+2;
             3*coord_ind+3];
res.coords = coords;%(res.coord_ind);%edit v2
%
res.sector_centers = int32(reshape(sector_centers,[3,sector_dim]));

% density compensation

if ~isempty(w)
    res.density_comp = w(:);%w(res.data_ind);%v2
else
    res.density_comp = [];
end
