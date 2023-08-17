# NTFS

본인의 NTFS 파일시스템을 분석해서 MFT의 cluster run 을 분석하고, 실제 디스크에서 클러스터를 어떻게 사용하고 있는지 출력해라

# NTFS

1. MFT0번을 어떻게 찾았는지.
2. MFT0번에 Fixup Array를 어떻게 수정했는지
3. MFT0번의 Data 속성을 어떻게 찾았는지
4. MFT0번의 Data 속성의 ClusterRun 정보는 어떻게 찾았는지
5. MFT0번의 ClusterRun 분석
6. 각각이 차지하는 실제 LCN을 나열하시오.
   ex) 10진수로 클러스터 값을 출력하면 됨.
   Cluster Start: 1230, Size: 30
   Cluster Start: 1800(절대위치여야 함.), Size: 60

[링크]
https://kali-km.tistory.com/entry/NTFS-File-System-5
