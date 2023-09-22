#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define arrSize 10000000

void main(int argc, char *argv[])
{
	int num_proc, my_rank;
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&num_proc);
	MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
	MPI_Status stat;

	int* arr = (int*)malloc(arrSize*sizeof(int));
	for(int j=0;j<arrSize;j++) arr[j] = 1;

	if (my_rank == 0)
	{
		int bnd = arrSize/(num_proc-1);
		int lw_bnd, up_bnd, sum;
		for(int i=1;i<num_proc;i++)
		{
			lw_bnd = (i-1)*bnd;
			up_bnd = i*bnd;
			if (i == num_proc-1) {up_bnd = arrSize;}
			MPI_Send(&lw_bnd,1,MPI_INT,i,101,MPI_COMM_WORLD);
			MPI_Send(&up_bnd,1,MPI_INT,i,102,MPI_COMM_WORLD);
		}
		MPI_Recv(&sum,1,MPI_INT,my_rank+1,103,MPI_COMM_WORLD,&stat);
		printf("Master received total sum = %d\n",sum);
	}
	else
	{
		int lw_bnd, up_bnd, rcv_sum, sum = 0;
		MPI_Recv(&lw_bnd,1,MPI_INT,0,101,MPI_COMM_WORLD,&stat);
		MPI_Recv(&up_bnd,1,MPI_INT,0,102,MPI_COMM_WORLD,&stat);
		printf("Process %d got lower bound %d and upper bound = %d\n",my_rank,lw_bnd,up_bnd);
		
		for(int y=lw_bnd;y<up_bnd;y++) sum = sum + arr[y];
		printf("Process %d calculated sum = %d\n",my_rank,sum);

		if (my_rank != num_proc-1)
		{
			MPI_Recv(&rcv_sum,1,MPI_INT,my_rank+1,103,MPI_COMM_WORLD,&stat);
			sum = sum + rcv_sum;	
		}
		
		printf("Cumulative sum at process %d = %d\n",my_rank,sum);
		MPI_Send(&sum,1,MPI_INT,my_rank-1,103,MPI_COMM_WORLD);
	}
	free(arr);
	MPI_Finalize();
}

