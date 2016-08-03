 
bcm        = 'RGGB'; 
blk = 64;
gain = 1; % 你看亮度自己设增益， 
% filename = 'E:\CEVA-XM4-prj\PreISP\Modules\RKMFNRv2\RKMFNRv2\data\test01-4164_3136_v241rkTest01837010611_16.raw';

filename = 'E:\CEVA-XM4-prj\PreISP\Modules\RKMFNRv2\RKMFNRv2\data\test13-4164_3136_v241rkTest01837010611_16.raw';
RawWid = 4164;
RawHgt = 3136;
fid = fopen(filename, 'rb');
RAW = fread(fid,[RawWid, RawHgt],'uint16');
RAW = RAW';       
fclose(fid);
WriteRaw2RGB_Demosaic(RAW, bcm, blk, gain, filename)

